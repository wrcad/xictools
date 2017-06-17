set verbose 1
read lef /usr/local/mrouter/examples/osu35/osu035_stdcells.lef
set via_stack 2
set vdd vdd
set gnd gnd
obstruction -6.4 183.0 246.4 186.0 metal1
obstruction -6.4 -6.0 246.4 1.0 metal1
obstruction -6.4 -6.0 0.8 186.0 metal1
obstruction 242.4 -6.0 246.4 186.0 metal1
obstruction 0.8 183.0 242.4 186.0 metal3
obstruction 0.8 -6.0 242.4 1.0 metal3
obstruction -6.4 1.0 0.8 183.0 metal2
obstruction 242.4 1.0 246.4 183.0 metal2
obstruction -6.4 1.0 0.8 183.0 metal4
obstruction 242.4 1.0 246.4 183.0 metal4
read def map9v3.def
stage1 -ma
stage2
append map9v3.def map9v3_routed.def
#write def map9v3_routed.def
