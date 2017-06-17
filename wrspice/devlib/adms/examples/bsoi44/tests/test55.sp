** PMOSFET: Benchmarking of B4SOI Id-Vd by Jane Xi 05/09/2003.
* Modified by Darsen Lu 03/11/2009
* Modified by Tanvir Morshed 11/21/2010

.option post nopage brief
.option ingold=1
.option gmin=0

m1 d g s 0 p p1 w=1u l=0.1u soimod=0 NF=1 
*+SA=0.31u SB=0.2u SD=0.1u 

vg g 0          -1.2
vd d 0          -1.2 
vs s 0 		0.0
vp p 0 		0.0

.dc vd 0 -1.2 -0.02 vg -0.4 -1.2 -0.1 
.include ./pmos4p4.mod
.print dc i(vd)

.control
run
plot -i(vd) vs v(d)
.endc

.end

