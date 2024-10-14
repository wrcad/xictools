 Mx  Drain Gate Source Back-gate(substrate) Body  Tx  W  L (body ommitted for FB)
* Modified by Darsen Lu 03/11/2009
* Modified by Tanvir Morshed 11/21/2010

.include ./nmos4p4.mod
.include ./pmos4p4.mod
.option TEMP=27C

Vpower VD 0 1.5
Vgnd VS 0 0
Vgate Gate 0 0.0
MN0 VS Gate Out VS N1 W=10u L=0.18u
MP0 VD Gate Out VS P1 W=20u L=0.18u

.dc Vgate 0 1.5 0.05
.print dc v(out)

.control
run
plot v(Out)
.endc

.END
