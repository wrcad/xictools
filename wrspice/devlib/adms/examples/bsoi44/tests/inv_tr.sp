Mx  Drain Gate Source Back-gate(substrate) Body  Tx  W  L (body ommitted for FB)
* Modified by Darsen Lu 03/11/2009
* Modified by Tanvir Morshed 11/21/2010

.include ./nmos4p4.mod
.include ./pmos4p4.mod
.option TEMP=27C

Vpower VD 0 1.5
Vgnd VS 0 0

Vgate   Gate   VS PULSE(0v 1.5v 100ps 50ps 50ps 200ps 500ps)

MN0 VS Gate Out VS N1 W=10u L=0.18u
MP0 VD Gate Out VS P1 W=20u L=0.18u

.tran 0.01n 600ps
.print tran v(gate) v(out)

.control
run
plot v(Gate) v(Out)
.endc

.END
