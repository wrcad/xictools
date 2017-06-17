
#
#   nexxim DC, AC and noise test routines
#

#  NOTE (09/11/2007): This module can only be run in a Linux environment and
#                     does not support other platforms yet.

#
# Vers  Date            Who             Comments
# ====  ==========      =============   ========
#  1.2  09/11/07        Greg Nofi       Bug fixes and other updates
#  1.1  07/03/07        Greg Nofi       Use sdf2csv for comparisons, not MATLAB
#  1.0  02/14/07        Greg Nofi       Initial version
#

package simulate;
$simulatorCommand=getSimulatorCommand();
$netlistFileExt=".scs";
use strict;

sub getSimulatorCommand {
#
# Which simulation command should be used? Depends on OS. Assumes that Linux
#  environment has script called nexxim in path to run sim.
#
    my($simCommand);
    my($osName) = kernal();
    if ($osName =~ /linux/i) {
        $simCommand = "nexxim";
    } elsif ($osName =~ /sunos/i || $osName =~ /win/i) {
        die("ERROR: nexxim.pm cannot run in $osName yet. It can only be run in Linux.");
    } else {
        die("ERROR: Cannot determine simulator command for OS $osName");
    }
    return $simCommand;
}

sub kernal {
#
#   This subroutine returns a string that includes the kernal (OS) name.
#
    use Config;
    my($osName) = $modelQa::Config{osname};

    if ($osName !~ /win/i) {
        open(UNAME,"uname -s|") or die("ERROR: cannot determine OS information, stopped");
        chomp($osName=<UNAME>);
    }
    return($osName);
}

sub version {
    my($version);
    $version="unknown";
    if (!open(SIMULATE,"$simulate::simulatorCommand -V 2>/dev/null|")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    while (<SIMULATE>) {
        chomp;
        if (s/^\s*Nexxim version:\s+//) {
            /([\d\.]+)/;
            $version=$1;
        }
    }
    close(SIMULATE);
    return($version);
}

sub runNoiseTest {
    my($variant,$outputFile)=@_;
    my($pin,$noisePin,$noiseOut,@BiasList,$n);
    my(@netlistfileparts,$netlistfile);
    my($temperature,$biasVoltage,$sweepVoltage);

#
#   Make up the netlist, using a subckt to encapsulate the
#   instance. This simplifies handling of the variants as
#   the actual instance is driven by voltage-controlled
#   voltage sources from the subckt pins, and the currents
#   are fed back to the subckt pins using current-controlled
#   current sources. Pin swapping, polarity reversal, and
#   m-factor scaling can all be handled by simple modifications
#   of this subckt.
#

    # Make netlist name = outputFile (without its dir path) plus netlistFile
    @netlistfileparts = split(/\//,$outputFile . $simulate::netlistFileExt);
    $netlistfile = $netlistfileparts[-1];

    $n=0;
    $noisePin=$main::Outputs[0];
    if ($main::isFloatingPin{$noisePin}) {
        $noiseOut = $noisePin;
    } else {
        $noiseOut = "n_$noisePin";
    }
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
            foreach $sweepVoltage (@main::BiasSweepList) {
                if (!open(OF,">$netlistfile")) {
                    die("ERROR: cannot open file $netlistfile, stopped");
                }
                print OF "* Noise simulation for $main::simulatorName";
                &generateCommonNetlistInfo($variant);
                @BiasList=split(/\s+/,$main::biasListSpec);
                foreach $pin (@main::Pin) {
                    if ($main::isFloatingPin{$pin}) {
                        print OF "i_$pin ($pin 0) isource dc=0";
                        print OF "save $pin";
                    } else {
                        if ($pin eq $main::biasListPin) {
                            print OF "v_$pin ($pin 0) vsource dc=$biasVoltage";
                        } elsif ($pin eq $main::biasSweepPin) {
                            print OF "v_$pin ($pin 0) vsource dc=$sweepVoltage";
                        } else {
                            print OF "v_$pin ($pin 0) vsource dc=$main::BiasFor{$pin}";
                        }
                        print OF "save v_$pin:p";
                    }
                }
                print OF "x1 (".join(" ",@main::Pin).") mysub";
                if (! $main::isFloatingPin{$noisePin}) {
                    print OF "fn (0 n_$noisePin) cccs probe=v_$noisePin gain=1";
                    print OF "rn (0 n_$noisePin) resistor r=1 isnoisy=no";
                }
                if ($main::fMin == $main::fMax) {
                    print OF "noise ($noiseOut) noise values=[$main::fMin]";
                } else {
                    print OF "noise ($noiseOut) noise start=$main::fMin stop=$main::fMax $main::fType=$main::fSteps";
                }
                print OF "tempopt options temp=$temperature";
                
                close(OF);

#
#   Run simulations
#
                if (!open(SIMULATE, "$simulate::simulatorCommand $netlistfile 2>/dev/null|")) {
                    die("ERROR: cannot run $main::simulatorName, stopped");
                }
                while (<SIMULATE>) {
                    chomp;
                }
                close(SIMULATE);
                if (-e "$netlistfile.sdf") {
                    sdf2csv("$netlistfile.sdf", "$netlistfile.sdf.csv.$n");
                    $n++;
                } else {
                    die("ERROR: Simulation result '$netlistfile.sdf' cannot be found.");
                }
              }
          }
    }

    processNoiseResult($netlistfile.".sdf.csv",$outputFile);
    cleanup($netlistfile);
}

sub processNoiseResult {
    my($csvfile,$outputFile)=@_;
    my(@headerparts,@lineparts,$outline);
    my(@X,%Noise,%Idx,$h,$i,$n,$pin);

#
#   Parse info from CSV files
#
    for ( $n = 0; -e "$csvfile.$n"; $n++ ) {
        if (! open(IF,"$csvfile.$n")) {
            die("ERROR: cannot open file $csvfile.$n, stopped");
        }
        # FORMAT:
        #  - 1st column: alter names; ignore that
        #  - 2nd column: frequency or voltage; goes into list @X
        #  - 3rd and beyond: output noise; parse header so that we know what
        #                     which output each column represents
        $_ = <IF>;
        chomp;
        @headerparts = split(/\,/);
        shift(@headerparts);
        for ( $h=1; $h < @headerparts; $h++ ) {
            if ($headerparts[$h] =~ /v\(n_(.+)\)/) {
                $Idx{$1} = $h;
            } else {
                die("Do not know how to handle CSV column '$headerparts[$h]'");
            }
        }

        # Parse remainder of CSV file for values
        while (<IF>) {
            chomp;
            @lineparts = split(/\,/);
            shift(@lineparts);
            push(@X, $lineparts[0]);
            foreach $pin (@main::Outputs) {
                push(@{$Noise{$pin}}, $lineparts[$Idx{$pin}]);
            }
        }
        close(IF);
        if (! $main::debug) {
            unlink("$csvfile.$n");
        }
    }

#
#   Write info parsed from CSV to a new result file
#
    if (!open(OF,">$outputFile")) {
        die("ERROR: cannot open file $outputFile, stopped");
    }
    if ($main::fMin == $main::fMax) {
        printf OF ("V($main::biasSweepPin)");
    } else {
         printf OF ("Freq");
    }
    foreach (@main::Outputs) {
        printf OF (" N($_)");
    }
    printf OF ("\n");
    for ( $i = 0; $i <= $#X; ++$i ) {
        $outline = $X[$i];
        foreach $pin (@main::Outputs) {
            if (defined(${Noise{$pin}})) {
                $outline .= " " . ${Noise{$pin}}[$i];
            }
        }
        printf OF "$outline\n";
    }
    close(OF);
}

sub runAcTest {
    my($variant,$outputFile)=@_;
    my($pin,%NextPin,@BiasList,$i);
    my($temperature,$biasVoltage,$sweepVoltage);
    my(@netlistfileparts,$netlistfile);

#
#   Make up the netlist, using a subckt to encapsulate the
#   instance. This simplifies handling of the variants as
#   the actual instance is driven by voltage-controlled
#   voltage sources from the subckt pins, and the currents
#   are fed back to the subckt pins using current-controlled
#   current sources. Pin swapping, polarity reversal, and
#   m-factor scaling can all be handled by simple modifications
#   of this subckt.
#

    @netlistfileparts = split(/\//,$outputFile . $simulate::netlistFileExt);
    $netlistfile = $netlistfileparts[-1];
    
    if (!open(OF,">$netlistfile")) {
        die("ERROR: cannot open file $netlistfile, stopped");
    }
    print OF "* AC simulation for $main::simulatorName";
    &generateCommonNetlistInfo($variant);
    @BiasList=split(/\s+/,$main::biasListSpec);
    print OF "simulator lang=spice";
    print OF ".param vbias=$BiasList[0]";
    print OF ".param vsweep=$main::BiasSweepList[0]";
    foreach $pin (@main::Pin) {
        if ($pin eq $main::Pin[0]) {
            print OF ".param ac_$pin=1";
        } else {
            print OF ".param ac_$pin=0";
        }
        if ($main::isFloatingPin{$pin}) {
            print OF "i_$pin $pin 0 0";
        } elsif ($pin eq $main::biasListPin) {
            print OF "v_$pin $pin 0 vbias ac ac_$pin";
        } elsif ($pin eq $main::biasSweepPin) {
            print OF "v_$pin $pin 0 vsweep ac ac_$pin";
        } else {
            print OF "v_$pin $pin 0 $main::BiasFor{$pin} ac ac_$pin";
        }
    }
    print OF "x1 ".join(" ",@main::Pin)." mysub";
    print OF "simulator lang=spectre";
    if ($main::fMin == $main::fMax) {
      print OF "ac ac values=[$main::fMin]";
    } else {
      print OF "ac ac start=$main::fMin stop=$main::fMax $main::fType=$main::fSteps";
    }
    print OF "simulator lang=spice";
    foreach $pin (@main::Pin) {print OF ".print ac ir(v_$pin) ii(v_$pin)"}
    $NextPin{$main::Pin[0]}=$main::Pin[$#main::Pin];
    for ($i=1;$i<=$#main::Pin;++$i) {
        $NextPin{$main::Pin[$i]}=$main::Pin[$i-1];
    }
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (@BiasList) {
            foreach $sweepVoltage (@main::BiasSweepList) {
                foreach $pin (@main::Pin) {
                    if ($temperature == $main::Temperature[0] && $biasVoltage == $BiasList[0]
                        && $sweepVoltage == $main::BiasSweepList[0] && $pin eq $main::Pin[0]) {
                        print OF ".temp $temperature";
                    } else {
                        print OF ".alter";
                        if ($biasVoltage == $BiasList[0] &&
                            $sweepVoltage == $main::BiasSweepList[0] &&
                            $pin eq $main::Pin[0]) {
                            print OF ".temp $temperature";
                        }
                        if ($sweepVoltage == $main::BiasSweepList[0] && $pin eq $main::Pin[0]) {
                            print OF ".param vbias=$biasVoltage";
                        }
                        if ($pin eq $main::Pin[0]) {
                            print OF ".param vsweep=$sweepVoltage";
                        }
                        print OF ".param ac_$NextPin{$pin}=0";
                        print OF ".param ac_$pin=1";
                    }
                }
            }
        }
    }
    print OF ".end";
    close(OF);

#
#   Run simulations and get the results
#
    if (!open(SIMULATE,"$simulate::simulatorCommand $netlistfile 2>/dev/null|")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    while (<SIMULATE>) {
        chomp;
    }
    close(SIMULATE);

#
#   Run command to convert simulation result into CSV file
#
    if (-e "$netlistfile.sdf") {
        sdf2csv("$netlistfile.sdf");
    } else {
        die("ERROR: Simulation result '$netlistfile.sdf' cannot be found.");
    }
    
    processAcResult("$netlistfile.sdf.csv",$outputFile);
    cleanup($netlistfile);
}

sub processAcResult {
    my($csvfile,$outputFile)=@_;
    my($type,$pin,$mPin,$fPin,@X,%g,%c);
    my($temperature,$biasVoltage);
    my(@headerparts,$h,%Idx,$m,%pinIdx,$p);
    my(@csv,@tmp,$row,$outline,$numsteps);
    my($twoPi)=8.0*atan2(1.0,1.0);

#
#   Initialize some post-processing info
#
    if (! open(IF,$csvfile)) {
        die("ERROR: cannot open file $csvfile, stopped");
    }

    foreach $mPin (@main::Pin) {
        foreach $fPin (@main::Pin) {
            @{$g{$mPin,$fPin}}=();
            @{$c{$mPin,$fPin}}=();
        }
    }

#
#   Post-processing
#
    $_ = <IF>;
    chomp;
    @headerparts = split(/\,/);
    shift(@headerparts);
    # Parse header row for column indices that correspond to output values
    for ($h=1;$h<@headerparts;$h++) {
        if ($headerparts[$h] =~ /ir\(I\(v_(.+)\)\)/) {
            $Idx{'g',$1} = $h;
        } elsif ($headerparts[$h] =~ /ii\(I\(v_(.+)\)\)/) {
            $Idx{'c',$1} = $h;
        }
    }

    if ($main::fMin == $main::fMax) {
        @X=();
        foreach $temperature (@main::Temperature) {
            foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
                push(@X,@main::BiasSweepList);
            }
        }
    }

    # Split each csv line into words and add to table. Do not save first column.
    while (<IF>) {
        chomp;
	@tmp = split(/\,/);
        shift(@tmp);
	push @csv, [ @tmp ];
    }
    close(IF);

#
#   Write the results to a file
#
    if (!open(OF,">$outputFile")) {
        die("ERROR: cannot open file $outputFile, stopped");
    }
    if ($main::fMin == $main::fMax) {
        printf OF ("V($main::biasSweepPin)");
    } else {
        printf OF ("Freq");
    }
    foreach (@main::Outputs) {
        ($type,$mPin,$fPin)=split(/\s+/,$_);
        printf OF (" $type($mPin,$fPin)");
    }
    printf OF ("\n");

    # Create hash table containing index into pin array
    $p = 0;
    foreach $pin (@main::Pin) {
        $pinIdx{$pin} = $p++;
    }

    if ($main::fMin == $main::fMax) {
        # Parse table in chunks of 4 row. Each 4 row set represents a sweep step.
        for ($m = 0; $m < @csv; $m += @main::Pin) {
            $outline = shift(@X);
            foreach (@main::Outputs) {
                ($type,$mPin,$fPin)=split(/\s+/,$_);
                $row = $m + $pinIdx{$fPin};
                if ($type eq 'g') {
                    $outline .= " $csv[$row][$Idx{$type,$mPin}]";
                } else {
                    $outline .= " " . abs($csv[$row][$Idx{$type,$mPin}]);
                }
            }
            print OF $outline;
        }
    } else {
        $numsteps = @csv / @main::Pin;
        for ($m = 0; $m < $numsteps; $m++) {
            $outline = $csv[$m][0];  # Frequency is first word in row
            foreach (@main::Outputs) {
                ($type,$mPin,$fPin)=split(/\s+/,$_);
                $row = $numsteps * $pinIdx{$fPin} + $m;
                $outline .= " " . abs($csv[$row][$Idx{$type,$mPin}]/($twoPi*$csv[$m][0]));
            }
            print OF $outline;
        }
    }
    close(OF);
}

sub runDcTest {
    my($variant,$outputFile)=@_;
    my($pin,$temperature,$biasVoltage);
    my(@BiasList,$start,$stop,$step);
    my(@netlistfileparts,$netlistfile);

#
#   Make up the netlist, using a subckt to encapsulate the
#   instance. This simplifies handling of the variants as
#   the actual instance is driven by voltage-controlled
#   voltage sources from the subckt pins, and the currents
#   are fed back to the subckt pins using current-controlled
#   current sources. Pin swapping, polarity reversal, and
#   m-factor scaling can all be handled by simple modifications
#   of this subckt.
#

    @netlistfileparts = split(/\//,$outputFile . $simulate::netlistFileExt);
    $netlistfile = $netlistfileparts[-1];
    if (!open(OF,">$netlistfile")) {
        die("ERROR: cannot open file $netlistfile, stopped");
    }
    print OF "* DC simulation for $main::simulatorName";
    &generateCommonNetlistInfo($variant);
    @BiasList=split(/\s+/,$main::biasListSpec);
    ($start,$stop,$step)=split(/\s+/,$main::biasSweepSpec);
    $start-=$step;
    print OF ".param vbias=$BiasList[0]";
    foreach $pin (@main::Pin) {
        if ($main::isFloatingPin{$pin}) {
            print OF "i_$pin $pin 0 0";
        } elsif ($pin eq $main::biasListPin) {
            print OF "v_$pin $pin 0 vbias";
        } elsif ($pin eq $main::biasSweepPin) {
            print OF "v_$pin $pin 0 $start";
        } else {
            print OF "v_$pin $pin 0 $main::BiasFor{$pin}";
        }
    }
    print OF "x1 ".join(" ",@main::Pin)." mysub";
    print OF ".dc v_$main::biasSweepPin $main::biasSweepSpec";

    foreach $pin (@main::Outputs) {
        if ($main::isFloatingPin{$pin}) {
            print OF ".print v($pin)"
        } else {
            print OF ".print i(v_$pin)"
        }
    }

    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (@BiasList) {
            if ($temperature == $main::Temperature[0] && $biasVoltage == $BiasList[0]) {
                print OF ".temp $temperature";
            } else {
                print OF ".alter";
                if ($biasVoltage == $BiasList[0]) {
                    print OF ".temp $temperature";
                }
                print OF ".param vbias=$biasVoltage";
            }
        }
    }
    print OF ".end";
    close(OF);

#
#   Run simulations
#
    if (!open(SIMULATE,"$simulate::simulatorCommand $netlistfile 2>/dev/null|")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    while (<SIMULATE>) {
    }
    close(SIMULATE);
    
#
#   Run command to convert simulation result into a CSV file
#
    if (-e "$netlistfile.sdf") {
        sdf2csv("$netlistfile.sdf");
    } else {
        die("ERROR: Simulation result \"$netlistfile.sdf\" cannot be found.");
    }

    processDcResult("$netlistfile.sdf.csv",$outputFile,$start,$step);
    cleanup($netlistfile);
}

sub processDcResult {
    my($csvfile,$outputFile,$start,$step)=@_;
    my($i,$pin,$h,$Vidx,%Idx);
    my(@V,%DC,@headerparts,@lineparts);

#
#   Read the CSV file
#
    if (! open(IF,"$csvfile")) {
        die("ERROR: cannot open file $csvfile, stopped");
    }
    @V=();
    foreach $pin (@main::Outputs) {
        @{$DC{$pin}}=();
        @{$Idx{$pin}}=();
    }
    $_ = <IF>;
    chomp;
    @headerparts = split(/\,/);
    shift(@headerparts);
    # Parse header row for column indices that correspond to the sweep voltage and result value
    for ($h=0;$h<@headerparts;$h++) {
        if ($headerparts[$h] =~ /v_$main::biasSweepPin\.dc/) {
            $Vidx = $h;
        } elsif ($headerparts[$h] =~ /I\(positive\(v_(.+)\)\)/) {
            $Idx{$1} = $h;
        }
    }
    # Parse remainder of CSV file for values
    while (<IF>) {
        chomp;
        @lineparts = split(/\,/);
        shift(@lineparts);
        push(@V,$lineparts[$Vidx]);
        foreach $pin (@main::Outputs) {
            push(@{$DC{$pin}}, $lineparts[$Idx{$pin}]);
        }
    }
    close(IF);

#
#   Write the results to a file
#

    if (!open(OF,">$outputFile")) {
        die("ERROR: cannot open file $outputFile, stopped");
    }
    printf OF ("V($main::biasSweepPin)");
    foreach $pin (@main::Outputs) {
        if ($main::isFloatingPin{$pin}) {
            printf OF (" V($pin)");
        } else {
            printf OF (" I($pin)");
        }
    }
    printf OF ("\n");
    for ($i=0;$i<=$#V;++$i) {
        next if (abs($V[$i]-$start) < abs(0.1*$step)); # this is dummy first bias point
        printf OF ("$V[$i]");
        foreach $pin (@main::Outputs) {printf OF (" ${$DC{$pin}}[$i]")}
        printf OF ("\n");
    }
    close(OF);
}

sub generateCommonNetlistInfo {
    my($variant)=$_[0];
    my(@Pin_x,$arg,$name,$value,$eFactor,$fFactor,$pin);
    
    # WORKAROUND (Aug 30 2007): Put in this option until QA GMIN becomes integrated into Nexxim (or not)
    print OF ".option qagmin=1e-12";

    if ($variant=~/^scale$/) {
        print OF "testOptions options scale=$main::scaleFactor";
    }
    if ($variant=~/^shrink$/) {
        print OF "testOptions options scale=".(1.0-$main::shrinkPercent*0.01);
    }
    if ($variant=~/_P/) {
        $eFactor=-1;$fFactor=1;
    } else {
        $eFactor=1;$fFactor=-1;
    }
    if ($variant=~/^m$/) {
        if ($main::outputNoise) {
            $fFactor/=sqrt($main::mFactor);
        } else {
            $fFactor/=$main::mFactor;
        }
    }
    if (defined($main::verilogaFile)) {
        print OF "ahdl_include \"$main::verilogaFile\"";
    }
    foreach $pin (@main::Pin) {push(@Pin_x,"${pin}_x")}
    print OF "subckt mysub (".join(" ",@Pin_x).")";
    foreach $pin (@main::Pin) {
        if ($main::isFloatingPin{$pin}) {
            if ($main::outputNoise && $pin eq $main::Outputs[0]) {
                if ($variant=~/^m$/) {
                    $eFactor=sqrt($main::mFactor);
                } else {
                    $eFactor=1;
                }
                print OF "e_$pin ${pin}_x 0 ${pin} 0 $eFactor";
            } else { # assumed "dt" thermal pin, no scaling sign change
                print OF "v_$pin (${pin} ${pin}_x) vsource dc=0";
            }
        } elsif ($variant=~/^Flip/ && defined($main::flipPin{$pin})) {
            ## WORKAROUND due to seg faults ##
            print OF "e_$pin (${pin} ${pin}1 $main::flipPin{$pin}_x 0) vcvs gain=$eFactor";
            print OF "e_${pin}_probe (${pin}1 0) iprobe";
            print OF "f_$pin ($main::flipPin{$pin}_x 0) cccs probe=e_${pin}_probe gain=(-1*$fFactor)";
            ## ORIGINAL CODE ##
            #print OF "e_$pin (${pin}   0 ${pin}_x 0) vcvs gain=$eFactor";
            #print OF "f_$pin (${pin}_x 0) cccs probe=e_$pin gain=$fFactor";
        } else {
            ## WORKAROUND due to seg faults ##
            print OF "e_$pin (${pin} ${pin}1 ${pin}_x 0) vcvs gain=$eFactor";
            print OF "e_${pin}_probe (${pin}1 0) iprobe";
            print OF "f_$pin (${pin}_x 0) cccs probe=e_${pin}_probe gain=(-1*$fFactor)";
            ## ORIGINAL CODE ##
            #print OF "e_$pin (${pin}   0 ${pin}_x 0) vcvs gain=$eFactor";
            #print OF "f_$pin (${pin}_x 0) cccs probe=e_$pin gain=$fFactor";
        }
    }
    if (defined($main::verilogaFile)) {
        if ($variant=~/_P/) {
            print OF "${main::keyLetter}1 ".join(" ",@main::Pin)." $main::pTypeSelectionArguments";
        } else {
            print OF "${main::keyLetter}1 ".join(" ",@main::Pin)." $main::nTypeSelectionArguments";
        }
        if ($variant=~/^scale$/) {
            print OF "+ scale=$main::scaleFactor";
        }
        if ($variant=~/^shrink$/) {
            print OF "+ shrink=$main::shrinkPercent";
        }
    } else {
        print OF "${main::keyLetter}1 (".join(" ",@main::Pin).") mymodel";
    }
    foreach $arg (@main::InstanceParameters) {
        ($name,$value)=split(/=/,$arg);
        if ($variant=~/^scale$/) {
            if ($main::isLinearScale{$name}) {
                $value/=$main::scaleFactor;
            } elsif ($main::isAreaScale{$name}) {
                $value/=$main::scaleFactor**2;
            }
        }
        if ($variant=~/^shrink$/) {
            if ($main::isLinearScale{$name}) {
                $value/=(1.0-$main::shrinkPercent*0.01);
            } elsif ($main::isAreaScale{$name}) {
                $value/=(1.0-$main::shrinkPercent*0.01)**2;
            }
        }
        print OF "+ $name=$value";
    }
    if ($variant eq "m") {
        print OF "+ m=$main::mFactor";
    }
    if (!defined($main::verilogaFile)) {
        if ($variant=~/_P/) {
            print OF "model mymodel $main::pTypeSelectionArguments";
        } else {
            print OF "model mymodel $main::nTypeSelectionArguments";
        }
    }
    foreach $arg (@main::ModelParameters) {
        print OF "+ $arg";
    }
    print OF "ends";
}

sub sdf2csv {
#
#   A wrapper for the system call to the sdf2csv executible, a Nexxim tool
#   It converts the Nexxim binary result SDF (1st arg) to a CSV file (2nd arg)
#   Name of output CSV file defaults to SDF name + .csv extension    
#
    my($sdffile,$csvfile)=@_;
    
    # First see if sdf2csv dir is in PATH. If not, add it.
    if ( index($ENV{"PATH"}, "sdf2csv") == -1 ) {
        $ENV{"PATH"} .= ":$ENV{'NEXXIM_INSTALL_DIR'}/sdf2csv";
    }

    # Run sdf2csv
    if ( !defined($csvfile) ) {
        $csvfile = "$sdffile.csv";
    }
    if (!open(SDF2CSV,"sdf2csv $sdffile -o $csvfile 2>/dev/null|")) {
        die("ERROR: cannot open file $sdffile, stopped");
    }
    while (<SDF2CSV>) {
        if (/Failed opening file $sdffile/) {
            die("ERROR: cannot open file $sdffile, stopped");
        } else {
            chomp;
        }            
    }
    close(SDF2CSV);
}  

sub cleanup {
#
#   Delete input & output files from simulation unless debug flag was specified
#
    my($netlist)=@_;
    if (! $main::debug) {
        unlink($netlist);
        unlink($netlist . '.sdf');
        unlink("$netlist.sdf.csv");
    }
}

1;
