#!/bin/bash
hspice -hdlpath ../Code -i test1.sp -o test1
hspice -hdlpath ../Code -i test2.sp -o test2
hspice -hdlpath ../Code -i test3.sp -o test3
hspice -hdlpath ../Code -i test4.sp -o test4
hspice -hdlpath ../Code -i test5.sp -o test5
hspice -hdlpath ../Code -i test6.sp -o test6
hspice -hdlpath ../Code -i test7.sp -o test7
hspice -hdlpath ../Code -i test8.sp -o test8

hspice -hdlpath ../Code -i inverter_dc.sp -o inverter_dc
hspice -hdlpath ../Code -i inverter_tr.sp -o inverter_tr
hspice -hdlpath ../Code -i ring_osc.sp -o ring_osc
rm -f *.ac* *.ic* *.pa* *.st* *.val *.sw* *.tr* *.sc* *.mt* *.ma* sxcmd.log acsymm.sp acsymm.lis *.out *.ms* *.valog
rm -rf *.pvadir *.ahdlSimDB
