
#
#   ads DC, AC and noise test routines
#

#
#  Rel  Date            Who             Comments
# ====  ==========      =============   ========
#  1.3  06/21/07        Colin McAndrew  Verilog-A model support added
#  1.2  06/30/06        Colin McAndrew  Floating node support added
#                                       Noise simulation added
#  1.0  04/13/06        Rick Poore/     Initial version
#                       Colin McAndrew
#

package simulate;
if (defined($main::simulatorCommand)) {
    $simulatorCommand=$main::simulatorCommand;
} else {
    $simulatorCommand="adssim";
}
$netlistFile="adsCkt";
$dummyVaFile="cmcQaDummy.va";
$mFactorVerilogaName="_M"; # for LRM2.2
use strict;

sub version {
    my($version,$vaVersion,@Field);
    $version="unknown";
    $vaVersion="unknown";
    if (!open(OF,">$simulate::dummyVaFile")) {
        die("ERROR: cannot open file $simulate::dummyVaFile, stopped");
    }
    print OF "";
    print OF "`include \"discipline.h\"";
    print OF "module dummy(p,n);";
    print OF "    inout      p,n;";
    print OF "    electrical p,n;";
    print OF "    analog begin";
    print OF "`ifdef __VAMS_COMPACT_MODELING__";
    print OF "        \$strobe(\"Verilog-A version is: LRM2.2\");";
    print OF "`else";
    print OF "        \$strobe(\"Verilog-A version is: LRM2.1\");";
    print OF "`endif";
    print OF "        I(p,n)  <+ V(p,n);";
    print OF "    end";
    print OF "endmodule";
    print OF "";
    close(OF);
    if (!open(OF,">$simulate::netlistFile")) {
        die("ERROR: cannot open file $simulate::netlistFile, stopped");
    }
    print OF "; Version determination simulation for ads";
    print OF "Options UseNutmegFormat=yes ASCII_Rawfile=yes";
    print OF "Options Temp=27";
    print OF "";
    print OF "#load \"veriloga\", \"$simulate::dummyVaFile\";";
    print OF "";
    print OF "define mysub (p n)";
    print OF "dummy:a1 p n \\";
    print OF "";
    print OF "end mysub";
    print OF "";
    print OF "V_Source:v_n n 0 Vdc=0";
    print OF "V_Source:v_p p 0 Vdc=0";
    print OF "mysub:x1 p n";
    print OF "SweepPlan:dcPlan Start=0 Stop=1 Step=1";
    print OF "DC:DC1 SweepVar=\"v_p.Vdc\" SweepPlan=\"dcPlan\"";
    close(OF);
    #if (!open(SIMULATE,"$simulate::simulatorCommand $simulate::netlistFile 2>&1|")) {
    if (!open(SIMULATE,"$simulate::simulatorCommand $simulate::netlistFile 2>/dev/null|")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    while (<SIMULATE>) {
        chomp;s/^\s+//;s/\s+$//;
        if (/HPEESOFSIM/i) {
            @Field=split;
            $version=$Field[2];
        }
        if (s/^\s*Verilog-A version is:\s*//i) {
            $vaVersion=$_;
            if ($vaVersion eq "LRM2.1") {
                $simulate::mFactorVerilogaName="m";  # for LRM2.1
            }
        }
    }
    close(SIMULATE);
    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink($simulate::dummyVaFile);
        unlink("$simulate::netlistFile.ds");
        unlink(".spiceinit");
        unlink("spectra.raw");
    }
    return($version,$vaVersion);
}

sub runNoiseTest {
    my($variant,$outputFile)=@_;
    my($i,@Field,$arg,$name,$value,$type,$pin,$noisePin);
    my(@BiasList,$realVal,$imagVal);
    my($start,$stop,$step,$sign,$inResults,%Index,$iVariables);
    my(@X,@Noise,$temperature,$biasVoltage,$sweepVoltage);
    my(@realAdsResults,@imagAdsResults);

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

    @X=();@Noise=();
    if ($main::outputNoise == 2) {
        $noisePin="$main::Outputs[0]_$main::Outputs[1]";
    } else {
        $noisePin=$main::Outputs[0];
    }
    ($start,$stop,$step)=split(/\s+/,$main::biasSweepSpec);
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
            if ($main::fMin == $main::fMax) {
                push(@X,@main::BiasSweepList);
            }
            if (!open(OF,">$simulate::netlistFile")) {
                die("ERROR: cannot open file $simulate::netlistFile, stopped");
            }
            print OF "; Noise simulation for $main::simulatorName";
            print OF "Options UseNutmegFormat=yes ASCII_Rawfile=yes";
            print OF "Options Temp=$temperature";
            print OF "Vsweep = 0 V";
            &generateCommonNetlistInfo($variant);
            foreach $pin (@main::Pin) {
                if ($main::isFloatingPin{$pin}) {
                    print OF "I_Source:i_$pin $pin 0 Idc=0";
                } elsif ($pin eq $main::biasListPin) {
                    if (defined($main::referencePinFor{$pin})) {
                        print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=$biasVoltage";
                        print OF "#uselib \"ckt\", \"VCVS\"";
                        print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                    } else {
                        print OF "V_Source:v_$pin $pin 0 Vdc=$biasVoltage";
                    }
                } elsif ($pin eq $main::biasSweepPin) {
                    if ($stop < $start) { # flip polarity as ADS always does lo->hi sweep
                        if (defined($main::referencePinFor{$pin})) {
                            print OF "V_Source:v_${pin} ${pin}_$main::referencePinFor{$pin} ${pin} Vdc=Vsweep";
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                        } else {
                            print OF "V_Source:v_$pin 0 $pin Vdc=Vsweep";
                        }
                    } else {
                        if (defined($main::referencePinFor{$pin})) {
                            print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=Vsweep";
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                        } else {
                            print OF "V_Source:v_$pin $pin 0 Vdc=Vsweep";
                        }
                    }
                } else {
                    if (defined($main::referencePinFor{$pin})) {
                        print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=$main::BiasFor{$pin}";
                        print OF "#uselib \"ckt\", \"VCVS\"";
                        print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                    } else {
                        print OF "V_Source:v_$pin $pin 0 Vdc=$main::BiasFor{$pin}";
                    }
                }
            }
            print OF "mysub:x_$noisePin ".join(" ",@main::Pin);
            if ($main::outputNoise == 2) {
                print OF "#uselib \"ckt\", \"VCVS\"";
                print OF "VCVS:e_$noisePin $noisePin 0 $main::Outputs[0] $main::Outputs[1] G=1";
            } elsif (! $main::isFloatingPin{$noisePin}) {
               print OF "SDD:fn_$noisePin 0 n_$noisePin I[1,0]=_c1 C[1]=\"v_$noisePin\"";
               print OF "R:r_$noisePin n_$noisePin 0 R=1 Noise=no";
            }
            print OF "OutputPlan:noiseOutput Type=\"Output\"";
            if ($main::fType eq "lin") {
                print OF "SweepPlan:noisePlan Start=$main::fMin Stop=$main::fMax Lin=$main::fSteps";
            } elsif ($main::fType eq "dec") {
                print OF "SweepPlan:noisePlan Start=$main::fMin Stop=$main::fMax Dec=$main::fSteps";
            } else { # octal sweep
                print OF "SweepPlan:noisePlan Start=$main::fMin Stop=$main::fMax Dec=".(int($main::fSteps*log(10)/log(2)));
            }
            if ($stop < $start) {
                $sign=-1;
                print OF "SweepPlan:dcPlan Start=".(-1)*$start." Stop=".(-1)*$stop." Step=".(-1)*$step;
            } else {
                $sign=1;
                print OF "SweepPlan:dcPlan Start=$start Stop=$stop Step=$step";
            }
            print OF "AC:AC1 CalcNoise=yes OutputPlan=\"noiseOutput\" SweepVar=\"freq\" \\";
            if ($main::outputNoise == 2 || $main::isFloatingPin{$noisePin}) {
                print OF "SweepPlan=\"noisePlan\" NoiseNode=\"$noisePin\"";
            } else {
                print OF "SweepPlan=\"noisePlan\" NoiseNode=\"n_$noisePin\"";
            }
            print OF "ParamSweep:Vsweep SimInstanceName=\"AC1\" SweepVar=\"Vsweep\" SweepPlan=\"dcPlan\"";
            close(OF);

#
#   Run simulations and get the results
#

            if (!open(SIMULATE,"$simulate::simulatorCommand $simulate::netlistFile 2>/dev/null |")) {
                die("ERROR: cannot run $main::simulatorName, stopped");
            }
            $inResults=0;
            while (<SIMULATE>) {
                chomp;
            }
            close(SIMULATE);
            if (!open(SIMULATE,"spectra.raw")) {
                die("ERROR: cannot open ADS spectra.raw file, stopped");
            }
            while (<SIMULATE>) {
                chomp;s/^\s+//;s/\s+$//;
                next if (/^$/);
                if (/Plotname:\s*AC/) {
                    $inResults=0;next;
                }
                if (/Plotname:\s*CT/) {
                    while (<SIMULATE>) {
                        chomp;s/^\s+//;s/\s+$//;
                        last if (/^$/);
                    }
                    next;
                }
                if (s/^Variables:\s*//) {
                    $iVariables=0;
                    @Field=split;
                    $Index{$Field[1]}=$Field[0];
                    while (<SIMULATE>) {
                        chomp;s/^\s+//;s/\s+$//;
                        if (/^Values:/) {
                            $inResults=1;last;
                        }
                        ++$iVariables;
                        @Field=split;
                        $Index{$Field[1]}=$Field[0];
                    }
                    @realAdsResults=();@imagAdsResults=();
                    next;
                }
                next if (!$inResults);
                s/,/ /;
                @Field=split;
                shift(@Field) if ($#Field == 2);
                push(@realAdsResults,$Field[0]);
                push(@imagAdsResults,$Field[1]);
                if ($#realAdsResults == $iVariables) {
                    if ($main::fMin != $main::fMax) {
                        push(@X,1*$realAdsResults[$Index{"freq"}]);
                    }
                    if ($main::outputNoise == 2 || $main::isFloatingPin{$noisePin}) {
                        push(@Noise,$realAdsResults[$Index{"$noisePin.noise"}]**2);
                    } else {
                        push(@Noise,$realAdsResults[$Index{"n_$noisePin.noise"}]**2);
                    }
                    @realAdsResults=();@imagAdsResults=();
                }
            }
            close(SIMULATE);
        }
    }

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
    if ($main::outputNoise == 2) {
        print OF (" N($main::Outputs[0],$main::Outputs[1])");
    } else {
        print OF (" N($main::Outputs[0])");
    }
    for ($i=0;$i<=$#X;++$i) {
        if (defined($Noise[$i])) {printf OF ("$X[$i] $Noise[$i]\n")}
    }
    close(OF);

#
#   Clean up, unless the debug flag was specified
#

    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink("$simulate::netlistFile.ds");
        unlink(".spiceinit");
        unlink("spectra.raw");
    }
}

sub runAcTest {
    my($variant,$outputFile)=@_;
    my($i,@Field,$arg,$name,$value,$type,$mPin,$fPin,%NextPin,$first_fPin);
    my(@BiasList,$acStim,$realVal,$imagVal,%sign);
    my($start,$stop,$step,$sign,$inResults,%Index,$iVariables);
    my(@X,$omega,%g,%c,%q,$temperature,$biasVoltage,$sweepVoltage,$twoPi);
    my(@realAdsResults,@imagAdsResults,$outputLine);
    $twoPi=8.0*atan2(1.0,1.0);

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

    foreach $mPin (@main::Pin) {
        if ($main::needAcStimulusFor{$mPin} && !defined($first_fPin)) {$first_fPin=$mPin}
        foreach $fPin (@main::Pin) {
            @{$g{$mPin,$fPin}}=();
            @{$c{$mPin,$fPin}}=();
            @{$q{$mPin,$fPin}}=();
        }
    }
    @X=();
    ($start,$stop,$step)=split(/\s+/,$main::biasSweepSpec);
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
            if ($main::fMin == $main::fMax) {
                push(@X,@main::BiasSweepList);
            }
            if (!open(OF,">$simulate::netlistFile")) {
                die("ERROR: cannot open file $simulate::netlistFile, stopped");
            }
            print OF "; AC simulation for $main::simulatorName";
            print OF "Options UseNutmegFormat=yes ASCII_Rawfile=yes";
            print OF "Options Temp=$temperature";
            print OF "Vsweep = 0 V";
            &generateCommonNetlistInfo($variant);
            foreach $fPin (@main::Pin) {
                next if (!$main::needAcStimulusFor{$fPin});
                foreach $mPin (@main::Pin) {
                    $sign{$mPin}=1.0;
                    if ($mPin eq $fPin) {
                        $acStim=" Vac=1";
                    } else {
                        $acStim="";
                    }
                    if ($main::isFloatingPin{$mPin}) {
                        print OF "I_Source:i_${mPin}_$fPin ${mPin}_$fPin 0 Idc=0";
                    } elsif ($mPin eq $main::biasListPin) {
                        if (defined($main::referencePinFor{$mPin})) {
                            print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin ${mPin}_${fPin}_$main::referencePinFor{$mPin} Vdc=$biasVoltage$acStim";
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_${mPin}_$fPin $main::referencePinFor{$mPin}_$fPin 0 ${mPin}_${fPin}_$main::referencePinFor{$mPin} 0 G=1";
                        } else {
                            print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin 0 Vdc=$biasVoltage$acStim";
                        }
                    } elsif ($mPin eq $main::biasSweepPin) {
                        if ($stop < $start) { # flip polarity as ADS always does lo->hi sweep
                            $acStim=~s/1/-1/;
                            $sign{$mPin}=-1.0;
                            if (defined($main::referencePinFor{$mPin})) {
                                print OF "V_Source:v_${mPin}_$fPin ${mPin}_${fPin}_$main::referencePinFor{$mPin} ${mPin}_$fPin Vdc=Vsweep$acStim";
                                print OF "#uselib \"ckt\", \"VCVS\"";
                                print OF "VCVS:e_${mPin}_$fPin $main::referencePinFor{$mPin}_$fPin 0 ${mPin}_${fPin}_$main::referencePinFor{$mPin} 0 G=1";
                            } else {
                                print OF "V_Source:v_${mPin}_$fPin 0 ${mPin}_$fPin Vdc=Vsweep$acStim";
                            }
                        } else {
                            if (defined($main::referencePinFor{$mPin})) {
                                print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin ${mPin}_${fPin}_$main::referencePinFor{$mPin} Vdc=Vsweep$acStim";
                                print OF "#uselib \"ckt\", \"VCVS\"";
                                print OF "VCVS:e_${mPin}_$fPin $main::referencePinFor{$mPin}_$fPin 0 ${mPin}_${fPin}_$main::referencePinFor{$mPin} 0 G=1";
                            } else {
                                print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin 0 Vdc=Vsweep$acStim";
                            }
                        }
                    } else {
                        if (defined($main::referencePinFor{$mPin})) {
                            print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin ${mPin}_${fPin}_$main::referencePinFor{$mPin} Vdc=$main::BiasFor{$mPin}$acStim";
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_${mPin}_$fPin $main::referencePinFor{$mPin}_$fPin 0 ${mPin}_${fPin}_$main::referencePinFor{$mPin} 0 G=1";
                        } else {
                            print OF "V_Source:v_${mPin}_$fPin ${mPin}_$fPin 0 Vdc=$main::BiasFor{$mPin}$acStim";
                        }
                    }
                }
                print OF "mysub:x_$fPin ".join("_$fPin ",@main::Pin)."_$fPin ";
            }
            if ($main::fType eq "lin") {
                print OF "SweepPlan:acPlan Start=$main::fMin Stop=$main::fMax Lin=$main::fSteps";
            } elsif ($main::fType eq "dec") {
                print OF "SweepPlan:acPlan Start=$main::fMin Stop=$main::fMax Dec=$main::fSteps";
            } else { # octal sweep
                print OF "SweepPlan:acPlan Start=$main::fMin Stop=$main::fMax Dec=".(int($main::fSteps*log(10)/log(2)));
            }
            if ($stop < $start) {
                $sign=-1;
                print OF "SweepPlan:dcPlan Start=".(-1)*$start." Stop=".(-1)*$stop." Step=".(-1)*$step;
            } else {
                $sign=1;
                print OF "SweepPlan:dcPlan Start=$start Stop=$stop Step=$step";
            }
            print OF "OutputPlan:acOutput Type=\"Output\" UseNodeNestLevel=yes NodeNestLevel=2 UseEquationNestLevel=yes EquationNestLevel=2";
            print OF "AC:AC1 OutputPlan=\"acOutput\" SweepVar=\"freq\" SweepPlan=\"acPlan\"";
            print OF "ParamSweep:Vsweep SimInstanceName=\"AC1\" SweepVar=\"Vsweep\" SweepPlan=\"dcPlan\"";
            close(OF);

#
#   Run simulations and get the results
#

            if (!open(SIMULATE,"$simulate::simulatorCommand $simulate::netlistFile 2>/dev/null |")) {
                die("ERROR: cannot run $main::simulatorName, stopped");
            }
            $inResults=0;
            while (<SIMULATE>) {
                chomp;
            }
            close(SIMULATE);
            if (!open(SIMULATE,"spectra.raw")) {
                die("ERROR: cannot open ADS spectra.raw file, stopped");
            }
            while (<SIMULATE>) {
                chomp;s/^\s+//;s/\s+$//;
                next if (/^$/);
                if (/Plotname:\s*AC/) {
                    $inResults=0;next;
                }
                if (/Plotname:\s*CT/) {
                    while (<SIMULATE>) {
                        chomp;s/^\s+//;s/\s+$//;
                        last if (/^$/);
                    }
                    next;
                }
                if (s/^Variables:\s*//) {
                    $iVariables=0;
                    @Field=split;
                    $Index{$Field[1]}=$Field[0];
                    while (<SIMULATE>) {
                        chomp;s/^\s+//;s/\s+$//;
                        if (/^Values:/) {
                            $inResults=1;last;
                        }
                        ++$iVariables;
                        @Field=split;
                        $Index{$Field[1]}=$Field[0];
                    }
                    @realAdsResults=();@imagAdsResults=();
                    next;
                }
                next if (!$inResults);
                s/,/ /;
                @Field=split;
                shift(@Field) if ($#Field == 2);
                push(@realAdsResults,$Field[0]);
                push(@imagAdsResults,$Field[1]);
                if ($#realAdsResults == $iVariables) {
                    if ($main::fMin != $main::fMax) {
                        push(@X,1*$realAdsResults[$Index{"freq"}]);
                    }
                    $omega=$twoPi*$realAdsResults[$Index{"freq"}];
                    foreach (@main::Outputs) {
                        ($type,$mPin,$fPin)=split(/\s+/,$_);
                        if ($type eq "g") {
                            push(@{$g{$mPin,$fPin}},$sign{$mPin}*$realAdsResults[$Index{"v_${mPin}_${fPin}.i"}]);
                        }
                        if ($type eq "c") {
                            if ($mPin eq $fPin) {
                                push(@{$c{$mPin,$fPin}},$sign{$mPin}*$imagAdsResults[$Index{"v_${mPin}_${fPin}.i"}]/$omega);
                            } else {
                                push(@{$c{$mPin,$fPin}},-$sign{$mPin}*$imagAdsResults[$Index{"v_${mPin}_${fPin}.i"}]/$omega);
                            }
                        }
                        if ($type eq "q") {
                            if (abs($realAdsResults[$Index{"v_${mPin}_${fPin}.i"}]) > 1.0e-99) {
                                push(@{$q{$mPin,$fPin}},$imagAdsResults[$Index{"v_${mPin}_${fPin}.i"}]/$realAdsResults[$Index{"v_${mPin}_${fPin}.i"}]);
                            } else {
                                push(@{$q{$mPin,$fPin}},1.0e99);
                            }
                        }
                    }
                    @realAdsResults=();@imagAdsResults=();
                }
            }
            close(SIMULATE);
        }
    }

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
    for ($i=0;$i<=$#X;++$i) {
        $outputLine="$X[$i]";
        foreach (@main::Outputs) {
            ($type,$mPin,$fPin)=split(/\s+/,$_);
            if ($type eq "g") {
                if (defined(${$g{$mPin,$fPin}}[$i])) {
                    $outputLine.=" ${$g{$mPin,$fPin}}[$i]";
                } else {
                    undef($outputLine);last;
                }
            } elsif ($type eq "c") {
                if (defined(${$c{$mPin,$fPin}}[$i])) {
                    $outputLine.=" ${$c{$mPin,$fPin}}[$i]";
                } else {
                    undef($outputLine);last;
                }
            } else {
                if (defined(${$q{$mPin,$fPin}}[$i])) {
                    $outputLine.=" ${$q{$mPin,$fPin}}[$i]";
                } else {
                    undef($outputLine);last;
                }
            }
        }
        if (defined($outputLine)) {printf OF ("$outputLine\n")}
    }
    close(OF);

#
#   Clean up, unless the debug flag was specified
#

    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink("$simulate::netlistFile.ds");
        unlink(".spiceinit");
        unlink("spectra.raw");
    }
}

sub runDcTest {
    my($variant,$outputFile)=@_;
    my($i,$arg,$name,$value,$pin);
    my($start,$stop,$step);
    my(@V,%DC,$temperature,$biasVoltage);
    my($inData,$inResults,$iVariables,@Field,%Index,@AdsResults,$sign);

    if (!defined($main::biasSweepPin)) {
        die("ERROR: biasSweep must be specified for a DC I(V) test, stopped");
    }

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

    @V=();
    foreach $pin (@main::Outputs) {@{$DC{$pin}}=()}
    ($start,$stop,$step)=split(/\s+/,$main::biasSweepSpec);
    $start-=$step;
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
            if (!open(OF,">$simulate::netlistFile")) {
                die("ERROR: cannot open file $simulate::netlistFile, stopped");
            }
            print OF "; DC simulation for $main::simulatorName";
            print OF "Options UseNutmegFormat=yes ASCII_Rawfile=yes";
            print OF "Options Temp=$temperature";
            &generateCommonNetlistInfo($variant);
            foreach $pin (@main::Pin) {
                if ($main::isFloatingPin{$pin}) {
                    print OF "I_Source:i_$pin $pin 0 Idc=0";
                } elsif ($pin eq $main::biasListPin) {
                    if (defined($main::referencePinFor{$pin})) {
                        print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=$biasVoltage";
                        print OF "#uselib \"ckt\", \"VCVS\"";
                        print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                    } else {
                        print OF "V_Source:v_$pin $pin 0 Vdc=$biasVoltage";
                    }
                } elsif ($pin eq $main::biasSweepPin) {
                    if ($stop < $start) { # flip polarity as ADS always does lo->hi sweep
                        if (defined($main::referencePinFor{$pin})) {
                            print OF "V_Source:v_${pin} ${pin}_$main::referencePinFor{$pin} ${pin} Vdc=".(-1)*$start;
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                        } else {
                            print OF "V_Source:v_$pin 0 $pin Vdc=".(-1)*$start;
                        }
                    } else {
                        if (defined($main::referencePinFor{$pin})) {
                            print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=$start";
                            print OF "#uselib \"ckt\", \"VCVS\"";
                            print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                        } else {
                            print OF "V_Source:v_$pin $pin 0 Vdc=$start";
                        }
                    }
                } else {
                    if (defined($main::referencePinFor{$pin})) {
                        print OF "V_Source:v_${pin} ${pin} ${pin}_$main::referencePinFor{$pin} Vdc=$main::BiasFor{$pin}";
                        print OF "#uselib \"ckt\", \"VCVS\"";
                        print OF "VCVS:e_$pin $main::referencePinFor{$pin} 0 ${pin}_$main::referencePinFor{$pin} 0 G=1";
                    } else {
                        print OF "V_Source:v_$pin $pin 0 Vdc=$main::BiasFor{$pin}";
                    }
                }
            }
            print OF "mysub:x1 ".join(" ",@main::Pin);
            if ($stop < $start) {
                $sign=-1;
                print OF "SweepPlan:dcPlan Start=".(-1)*$start." Stop=".(-1)*$stop." Step=".(-1)*$step;
            } else {
                $sign=1;
                print OF "SweepPlan:dcPlan Start=$start Stop=$stop Step=$step";
            }
            print OF "DC:DC1 SweepVar=\"v_$main::biasSweepPin.Vdc\" SweepPlan=\"dcPlan\"";
            close(OF);

#
#   Run simulations and get the results
#

            if (!open(SIMULATE,"$simulate::simulatorCommand $simulate::netlistFile 2>/dev/null |")) {
                die("ERROR: cannot run $main::simulatorName, stopped");
            }
            $inResults=0;
            while (<SIMULATE>) {
                chomp;
            }
            close(SIMULATE);
            if (!open(SIMULATE,"spectra.raw")) {
                die("ERROR: cannot open ADS spectra.raw file, stopped");
            }
            while (<SIMULATE>) {
                chomp;s/^\s+//;s/\s+$//;
                if (s/^Variables:\s*//) {
                    $iVariables=0;
                    @Field=split;
                    $Index{$Field[1]}=$Field[0];
                    while (<SIMULATE>) {
                        chomp;s/^\s+//;s/\s+$//;
                        if (/^Values:/) {
                            $inResults=1;last;
                        }
                        ++$iVariables;
                        @Field=split;
                        $Index{$Field[1]}=$Field[0];
                    }
                    @AdsResults=();
                    next;
                }
                next if (!$inResults);
                @Field=split;
                shift(@Field) if ($#Field == 2);
                push(@AdsResults,@Field);
                if ($#AdsResults == $iVariables) {
                    push(@V,$sign*$AdsResults[$Index{"v_$main::biasSweepPin.Vdc"}]);
                    foreach $pin (@main::Outputs) {
                        if ($pin eq $main::biasSweepPin) {
                            push(@{$DC{$pin}},$sign*$AdsResults[$Index{"v_$pin.i"}]);
                        } elsif ($main::isFloatingPin{$pin}) {
                            push(@{$DC{$pin}},1*$AdsResults[$Index{"$pin"}]);
                        } else {
                            push(@{$DC{$pin}},1*$AdsResults[$Index{"v_$pin.i"}]);
                        }
                    }
                    @AdsResults=();
                }
            }
            close(SIMULATE);
        }
    }

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

#
#   Clean up, unless the debug flag was specified
#

    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink("$simulate::netlistFile.ds");
        unlink(".spiceinit");
        unlink("spectra.raw");
    }
}

sub generateCommonNetlistInfo {
    my($variant)=$_[0];
    my(@Pin_x,$arg,$name,$value,$eFactor,$fFactor,$pin,@Args);
    foreach $pin (@main::Pin) {push(@Pin_x,"${pin}_x")}
    if ($variant=~/^scale$/) {
        die("ERROR: there is no scale or shrink option for ads, stopped");
    }
    if ($variant=~/^shrink$/) {
        die("ERROR: there is no scale or shrink option for ads, stopped");
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
        print OF "";
        print OF "#load \"veriloga\", \"$main::verilogaFile\";";
    }
    print OF " ";
    print OF "define mysub (".join(" ",@Pin_x).")";
    foreach $pin (@main::Pin) {
        if ($main::isFloatingPin{$pin}) {
            if ($main::outputNoise && $pin eq $main::Outputs[0]) {
                if ($variant=~/^m$/) {
                    $eFactor=sqrt($main::mFactor);
                } else {
                    $eFactor=1;
                }
                print OF "#uselib \"ckt\", \"VCVS\"";
                print OF "VCVS:e_$pin ${pin} 0 ${pin}_x 0 G=$eFactor";
            } else { # assumed "dt" thermal pin, no scaling sign change
                print OF "V_Source:v_$pin ${pin} ${pin}_x Vdc=0";
            }
        } elsif ($variant=~/^Flip/ && defined($main::flipPin{$pin})) {
            print OF "#uselib \"ckt\", \"VCVS\"";
            print OF "VCVS:e_$pin $main::flipPin{$pin}_x 0 ${pin}_v 0 G=$eFactor";
            print OF "V_Source:v_$pin ${pin}_v ${pin} Vdc=0";
            print OF "SDD:f_$pin $main::flipPin{$pin}_x 0 C[1]=\"v_$pin\" I[1]=_c1*$fFactor";
        } else {
            print OF "#uselib \"ckt\", \"VCVS\"";
            print OF "VCVS:e_$pin ${pin}_x 0 ${pin}_v 0 G=$eFactor";
            print OF "V_Source:v_$pin ${pin}_v ${pin} Vdc=0";
            print OF "SDD:f_$pin ${pin}_x 0 C[1]=\"v_$pin\" I[1]=_c1*$fFactor";
        }
    }
    print OF " ";
    if (defined($main::verilogaFile)) {
        if ($variant=~/_P/) {
            @Args=split(/\s+/,$main::pTypeSelectionArguments);
        } else {
            @Args=split(/\s+/,$main::nTypeSelectionArguments);
        }
        print OF "$Args[0]:${main::keyLetter}1 ".join(" ",@main::Pin)." \\";
        foreach $arg (@Args[1..$#Args]) {
            ($name,$value)=split(/=/,$arg);
            print OF "  ".$name."=$value \\";
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
            print OF "  ".$name."=$value \\";
        }
        if ($variant eq "m") {
            print OF "  $simulate::mFactorVerilogaName=$main::mFactor \\";
        }
        foreach $arg (@main::ModelParameters) {
            print OF "  ".$arg." \\";
        }
    } else {
        print OF "mymodel:${main::keyLetter}1 ".join(" ",@main::Pin)." \\";
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
            print OF "  ".ucfirst($name)."=$value \\";
        }
        if ($variant eq "m") {
            print OF "  _M=$main::mFactor \\";
        }
        print OF " ";
        if ($variant=~/_P/) {
            print OF "model mymodel $main::pTypeSelectionArguments \\";
        } else {
            print OF "model mymodel $main::nTypeSelectionArguments \\";
        }
        foreach $arg (@main::ModelParameters) {
            print OF "  ".ucfirst($arg)." \\";
        }
    }
    print OF " ";
    print OF "end mysub";
    print OF " ";
}

1;
