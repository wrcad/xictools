
#
#   eldo DC, AC and noise test routines
#

#
#  Rel  Date         Who                Comments
# ====  ==========   =============      ========
#  1.0  06/21/07     Yousry Elmaghraby  Initial release
#                    Rob Jones          for modelQa release 1.3
#

package simulate;
if (defined($main::simulatorCommand)) {
    $simulatorCommand=$main::simulatorCommand;
} else {
    $simulatorCommand="eldo";
}
$netlistFile="eldoCkt";
use strict;

sub version {
    my($version,$vaVersion);
    $va="unknown";
    $vaVersion="unknown";
    if (!open(SIMULATE,"$simulate::simulatorCommand -v 2>/dev/null|")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    while (<SIMULATE>) {
        chomp;
        if (/^Eldo\s+VERSION\s*:\s*ELDO\s+(\S+)\s+/) {
            $version=$1;
        }
    }
    close(SIMULATE);
    return($version,$vaVersion);
}

sub runNoiseTest {
    my($variant,$outputFile)=@_;
    my($arg,$name,$value,$type,$pin,$noisePin);
    my(@BiasList,$i,@Field);
    my(@X,@Noise,$temperature,$biasVoltage,$sweepVoltage,$inData);

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
    $noisePin=$main::Outputs[0];
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
            if ($main::fMin == $main::fMax) {
                push(@X,@main::BiasSweepList);
            }
            foreach $sweepVoltage (@main::BiasSweepList) {
                if (!open(OF,">$simulate::netlistFile")) {
                    die("ERROR: cannot open file $simulate::netlistFile, stopped");
                }
                print OF "* Noise simulation for $main::simulatorName";
                &generateCommonNetlistInfo($variant,$temperature);
                print OF "vin dummy 0 0 ac 1";
                print OF "rin dummy 0 rmod";
                print OF ".model rmod r res=1 noise=0";
                foreach $pin (@main::Pin) {
                    if ($main::isFloatingPin{$pin}) {
                        print OF "i_$pin $pin 0 0";
                    } elsif ($pin eq $main::biasListPin) {
                        print OF "v_$pin $pin 0 $biasVoltage";
                    } elsif ($pin eq $main::biasSweepPin) {
                        print OF "v_$pin $pin 0 $sweepVoltage";
                    } else {
                        print OF "v_$pin $pin 0 $main::BiasFor{$pin}";
                    }
                }
                print OF "x1 ".join(" ",@main::Pin)." mysub";
                if (! $main::isFloatingPin{$noisePin}) {
                    print OF "fn 0 n_$noisePin v_$noisePin 1";
                    print OF "rn 0 n_$noisePin rmod";
                }
                print OF ".ac $main::frequencySpec";
                if (! $main::isFloatingPin{$noisePin}) {
                    print OF ".noise v($noisePin) vin 1";
                } else {
                    print OF ".noise v(n_$noisePin) vin 1";
                }
                print OF ".print noise onoise";
                print OF ".end";
                close(OF);

#
#   Run simulations and get the results
#

                system("$simulate::simulatorCommand $simulate::netlistFile -k /home/model/doc/license/cle.cle > /dev/null");
                if (!open(SIMULATE,"<$simulate::netlistFile.chi")) {
                    die("ERROR: cannot run $main::simulatorName, stopped");
                }
                $inData=0;
                while (<SIMULATE>) {
                    chomp;s/^\s+//;s/\s+$//;@Field=split;
                    if (/HERTZ\s+ONOISE/i) {$inData=1;<SIMULATE>;<SIMULATE>;next}
                    if ($#Field != 1) {$inData=0;}
                    next if (!$inData);
                    if ($main::fMin != $main::fMax) {
                        push(@X,&modelQa::unScale($Field[0]));
                    }
                    push(@Noise,(&modelQa::unScale($Field[1]))**2);
                }

                close(SIMULATE);
            }
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
        printf OF (" N($_)");
    }
    printf OF ("\n");
    for ($i=0;$i<=$#X;++$i) {
        if (defined($Noise[$i])) {printf OF ("$X[$i] $Noise[$i]\n")}
    }
    close(OF);

#
#   Clean up, unless the debug flag was specified
#

    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink("$simulate::netlistFile.st0");
        unlink("$simulate::netlistFile.chi");
        unlink("$simulate::netlistFile.wdb");
        unlink("$simulate::netlistFile.id");
        unlink("simout.tmp");
        if (defined($main::verilogaFile)) {
            $_=$main::verilogaFile;
            s/\/\S+\///;
            s/(\S+)\.va//;
            unlink("$1.ai");
            unlink("$1.info");
            unlink("$1.log");
        }
        if (!opendir(DIRQA,".")) {
            die("ERROR: cannot open directory ., stopped");
        }
        foreach (grep(/^$simulate::netlistFile\.ic/,readdir(DIRQA))) {unlink($_)}
        closedir(DIRQA);
    }
}

sub runAcTest {
    my($variant,$outputFile)=@_;
    my($arg,$name,$value,$type,$pin,$mPin,$fPin,%NextPin);
    my(@BiasList,$i,@Field, $old_freq);
    my(@X,$omega,%g,%c,$twoPi,$temperature,$biasVoltage,$sweepVoltage);
    my($inData,$inResults,$outputLine);
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

    if (!open(OF,">$simulate::netlistFile")) {
        die("ERROR: cannot open file $simulate::netlistFile, stopped");
    }
    print OF "* AC simulation for $main::simulatorName";
    &generateCommonNetlistInfo($variant,$main::Temperature[0]);
    @BiasList=split(/\s+/,$main::biasListSpec);
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
    print OF ".ac $main::frequencySpec";
    foreach $pin (@main::Pin) {print OF ".print ac ir(v_$pin) ii(v_$pin)"}
    $NextPin{$main::Pin[0]}=$main::Pin[$#main::Pin];
    for ($i=1;$i<=$#main::Pin;++$i) {
        $NextPin{$main::Pin[$i]}=$main::Pin[$i-1];
    }
    foreach $temperature (@main::Temperature) {
        foreach $biasVoltage (@BiasList) {
            foreach $sweepVoltage (@main::BiasSweepList) {
                foreach $pin (@main::Pin) {
                    next if ($temperature == $main::Temperature[0] && $biasVoltage == $BiasList[0]
                             && $sweepVoltage == $main::BiasSweepList[0] && $pin eq $main::Pin[0]);
                    print OF ".alter";
                    if ($biasVoltage == $BiasList[0] && $sweepVoltage == $main::BiasSweepList[0] && $pin eq $main::Pin[0]) {
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
    print OF ".end";
    close(OF);

#
#   Run simulations and get the results
#

    foreach $mPin (@main::Pin) {
        foreach $fPin (@main::Pin) {
            @{$g{$mPin,$fPin}}=();
            @{$c{$mPin,$fPin}}=();
        }
    }
    for ($i=0;$i<$#main::Pin;++$i) {
        $NextPin{$main::Pin[$i]}=$main::Pin[$i+1];
    }
    $NextPin{$main::Pin[$#main::Pin]}=$main::Pin[0];
    system("$simulate::simulatorCommand $simulate::netlistFile -k /home/model/doc/license/cle.cle > /dev/null");
    if (!open(SIMULATE,"<$simulate::netlistFile.chi")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    $inData=0;$inResults=0;
    if ($main::fMin == $main::fMax) {
        @X=();
        foreach $temperature (@main::Temperature) {
            foreach $biasVoltage (split(/\s+/,$main::biasListSpec)) {
                push(@X,@main::BiasSweepList);
            }
        }
    }
    $fPin=$main::Pin[0];
    $old_freq="null";
    while (<SIMULATE>) {
        chomp;
        if (/AC\s+ANALYSIS/i) {$inResults=1;$inData=0;next}
        if (/Job end at/)     {$inResults=0;$fPin=$NextPin{$fPin}}
        if (/ACCOUNTING INFORMATION/) {$inResults=0;$fPin=$NextPin{$fPin}}
        next if (!$inResults);
        s/^\s+//;s/\s+$//;
        if (/^HERTZ\s*\S*\(V_([a-zA-z][a-zA-Z0-9]*)\)/) {$mPin=$1;$inData=1;<SIMULATE>;<SIMULATE>;next;}
        @Field=split;
        if ($#Field != 2
            || &modelQa::unScale($Field[0]) !~ /^($main::number)$/
            || &modelQa::unScale($Field[1]) !~ /^($main::number)$/
            || &modelQa::unScale($Field[2]) !~ /^($main::number)$/) {
            $inData=0;$old_freq="null";next;
        }
        if ($old_freq eq $Field[0]) {next;}
        next if (! $inData);
        if (($main::fMin != $main::fMax) && (lc($mPin) eq lc($main::Pin[0])) && (lc($mPin) eq lc($fPin))) {
            push(@X,&modelQa::unScale($Field[0]));
        }
        $omega=$twoPi*&modelQa::unScale($Field[0]);
        push(@{$g{lc($mPin),lc($fPin)}},&modelQa::unScale($Field[1]));
        if (lc($mPin) eq lc($fPin)) {
            push(@{$c{lc($mPin),lc($fPin)}},&modelQa::unScale($Field[2])/$omega);
        } else {
            push(@{$c{lc($mPin),lc($fPin)}},-1*&modelQa::unScale($Field[2])/$omega);
        }
        $old_freq=$Field[0];
    }
    close(SIMULATE);

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
            } else {
                if (defined(${$c{$mPin,$fPin}}[$i])) {
                    $outputLine.=" ${$c{$mPin,$fPin}}[$i]";
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
        unlink("$simulate::netlistFile.st0");
        unlink("$simulate::netlistFile.chi");
        unlink("$simulate::netlistFile.wdb");
        unlink("$simulate::netlistFile.id");
        if (!opendir(DIRQA,".")) {
            die("ERROR: cannot open directory ., stopped");
        }
        foreach (grep(/^$simulate::netlistFile\.ic/,readdir(DIRQA))) {unlink($_)}
        closedir(DIRQA);
        unlink("eldo.errors");
        unlink("simout.tmp");
        if (defined($main::verilogaFile)) {
            $_=$main::verilogaFile;
            s/\/\S+\///;
            s/(\S+)\.va//;
            unlink("$1.ai");
            unlink("$1.info");
            unlink("$1.log");
        }
    }
}

sub runDcTest {
    my($variant,$outputFile)=@_;
    my($arg,$name,$value,$i,$pin,@Field);
    my(@BiasList,$start,$stop,$step);
    my(@V,%DC,$temperature,$biasVoltage);
    my($inData,$inResults);

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

    if (!open(OF,">$simulate::netlistFile")) {
        die("ERROR: cannot open file $simulate::netlistFile, stopped");
    }
    print OF "* DC simulation for $main::simulatorName";
    &generateCommonNetlistInfo($variant,$main::Temperature[0]);
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
            next if ($temperature == $main::Temperature[0] && $biasVoltage == $BiasList[0]);
            print OF ".alter";
            if ($biasVoltage == $BiasList[0]) {
                print OF ".temp $temperature";
            }
            print OF ".param vbias=$biasVoltage";
        }
    }
    print OF ".end";
    close(OF);

#
#   Run simulations and get the results
#

    system("$simulate::simulatorCommand $simulate::netlistFile -k /home/model/doc/license/cle.cle > /dev/null");
    if (!open(SIMULATE,"<$simulate::netlistFile.chi")) {
        die("ERROR: cannot run $main::simulatorName, stopped");
    }
    $inData=0;$inResults=0;@V=();
    foreach $pin (@main::Outputs) {@{$DC{$pin}}=()}
    while (<SIMULATE>) {
        chomp;
        if (/DC\s+TRANSFER\s+CURVES/i) {$inResults=1;$inData=0;next}
        if (/Job end at/) {$inResults=0}
        if (/ACCOUNTING INFORMATION/) {$inResults=0}
        next if (!$inResults);
        s/^\s+//;s/\s+$//;
        if (/^V\S*\s+(I|V)\(V_(\S*)\)/) {
            chomp($_=$2);$pin=$_;$inData=1;<SIMULATE>;<SIMULATE>;next;
        }
        @Field=split;
        if ($#Field != 1
            || &modelQa::unScale($Field[0]) !~ /^($main::number)$/
            || &modelQa::unScale($Field[1]) !~ /^($main::number)$/) {
            $inData=0;
            next;
        }
        if (lc($pin) eq lc($main::Outputs[0])) {
            push(@V,&modelQa::unScale($Field[0]));
        }
        push(@{$DC{lc($pin)}},&modelQa::unScale($Field[1]));
    }
    close(SIMULATE);

#
#   Write the results to a file
#

    if (!open(OF,">$outputFile")) {
        die("ERROR: cannot open file $outputFile, stopped");
    }
    printf OF ("V($main::biasSweepPin)");
    foreach $pin (@main::Outputs) {
        if ($main::isFloatingPin{$pin}) {
            print OF " V($pin)";
        } else {
            print OF " I($pin)";
        }
    }
    for ($i=0;$i<=$#V;++$i) {
        next if (abs($V[$i]-$start) < abs(0.1*$step)); # this is dummy first bias point
        printf OF ("$V[$i]");
        foreach $pin (@main::Outputs) {
                printf OF (" ${$DC{$pin}}[$i]");
        }
        printf OF ("\n");
    }
    close(OF);

#
#   Clean up, unless the debug flag was specified
#

    if (! $main::debug) {
        unlink($simulate::netlistFile);
        unlink("$simulate::netlistFile.st0");
        unlink("$simulate::netlistFile.chi");
        unlink("$simulate::netlistFile.wdb");
        unlink("$simulate::netlistFile.id");
        if (!opendir(DIRQA,".")) {
            die("ERROR: cannot open directory ., stopped");
        }
        foreach (grep(/^$simulate::netlistFile\.ic/,readdir(DIRQA))) {unlink($_)}
        closedir(DIRQA);
        unlink("eldo.errors");
        unlink("simout.tmp");
        if (defined($main::verilogaFile)) {
            $_=$main::verilogaFile;
            s/\/\S+\///;
            s/(\S+)\.va//;
            unlink("$1.ai");
            unlink("$1.info");
            unlink("$1.log");
        }
    }
}

sub generateCommonNetlistInfo {
    my($variant,$temperature)=@_;
    my(@Pin_x,$arg,$name,$value,$eFactor,$fFactor,$pin,$vlaName,@SelectionArgs);
    print OF ".option numdgt=6";
    print OF ".option gmin=1e-30";
    print OF ".option compat";
    print OF ".option tnom=27";
    print OF ".temp $temperature";
    if ($variant=~/^scale$/) {
        print OF ".option scale=$main::scaleFactor";
    }
    if ($variant=~/^shrink$/) {
        print OF ".option scale=".(1.0-$main::shrinkPercent*0.01);
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
        print OF ".verilog $main::verilogaFile";
    }
    foreach $pin (@main::Pin) {push(@Pin_x,"${pin}_x")}
    print OF ".subckt mysub ".join(" ",@Pin_x);
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
                print OF "v_$pin ${pin} ${pin}_x 0";
            }
        } elsif ($variant=~/^Flip/ && defined($main::flipPin{$pin})) {
            print OF "e_$pin ${pin}_v 0 $main::flipPin{$pin}_x 0 $eFactor";
            print OF "v_$pin ${pin}_v ${pin} 0";
            print OF "f_$pin $main::flipPin{$pin}_x 0 v_$pin   $fFactor";
        } else {
            print OF "e_$pin ${pin}_v 0 ${pin}_x 0 $eFactor";
            print OF "v_$pin ${pin}_v ${pin} 0";
            print OF "f_$pin ${pin}_x 0 v_$pin   $fFactor";
        }
    }
    if (defined($main::verilogaFile)) {
        if ($variant=~/_P/) {
            ($vlaName,@SelectionArgs)=split(/\s+/,$main::pTypeSelectionArguments);
        } else {
            ($vlaName,@SelectionArgs)=split(/\s+/,$main::nTypeSelectionArguments);
        }
        if (($#SelectionArgs>=0) && ($SelectionArgs[0]!~/^(generic:|param:)$/i) ) {
            splice(@SelectionArgs,0,0,"generic:");
        }
        print OF "Y1 $vlaName ".join(" ",@main::Pin,@SelectionArgs);
        if ($variant=~/^scale$/) {
            print OF "+ scale=$main::scaleFactor";
        }
        if ($variant=~/^shrink$/) {
            print OF "+ shrink=$main::shrinkPercent";
        }
    } else {
        print OF "${main::keyLetter}1 ".join(" ",@main::Pin)." MYMODEL";
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
    if (defined($main::verilogaFile)) {
        print OF ".model $vlaName macro lang=veriloga ".join(" ",@SelectionArgs);
    } else {
        if ($variant=~/_P/) {
            print OF ".model MYMODEL $main::pTypeSelectionArguments";
        } else {
            print OF ".model MYMODEL $main::nTypeSelectionArguments";
       }
    }
    foreach $arg (@main::ModelParameters) {
        print OF "+ $arg";
    }
    print OF ".ends";
}

1;
