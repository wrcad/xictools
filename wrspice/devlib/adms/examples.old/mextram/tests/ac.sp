*** AC analysis of Mextram 504 model ***
 
.options gmin=1e-13 $ POST=1 converge=1 relv=1.e-6 absv=1.e-9
 
QCKT 11 22 33 44 m504 $ area=1.0 m=1
 
* START SOURCES
VE 3 0 DC 0
VB 2 0 DC 0.7
VC 1 0 DC 1.0
VS 4 0 DC 0
 
.DC VB 0.7 1.0 0.02
.ac dec 10 1.e4 1.e11
 
vc1 11 1 0
vb1 22 2 0 ac=0.001
ve1 33 3 0
vs1 44 4 0
 
.op
.print ac ir(vc1) ii(vc1) ir(vb1) ii(vb1)
 
.temp 25
 
.include mcard_504
 
.control
run
plot ysep ir(vc1) ii(vc1) ir(vb1) ii(vb1)
.endc
.END

