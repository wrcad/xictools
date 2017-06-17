set verbose 1
read lef /usr/local/mrouter/examples/osu35/osu035_stdcells.lef
set via_stack 2
set vdd vdd
set gnd gnd
obstruction -6.4 1583.0 2147.2 1586.0 metal1
obstruction -6.4 -6.0 2147.2 1.0 metal1
obstruction -6.4 -6.0 0.8 1586.0 metal1
obstruction 2143.2 -6.0 2147.2 1586.0 metal1
obstruction 0.8 1583.0 2143.2 1586.0 metal3
obstruction 0.8 -6.0 2143.2 1.0 metal3
obstruction -6.4 1.0 0.8 1583.0 metal2
obstruction 2143.2 1.0 2147.2 1583.0 metal2
obstruction -6.4 1.0 0.8 1583.0 metal4
obstruction 2143.2 1.0 2147.2 1583.0 metal4
read def openMSP430.def
stage1 -ma
stage2
#append openMSP430.def openMSP430_routed.def
write def openMSP430_routed.def
