*** Transient analysis of Mextram 504 model ***
 
.options gmin=1e-13 $ dccap POST=1
 
QCKT 1 2 0 0 m504 $ area=1.0 m=1
 
* START SOURCES
 
VC 3 0 DC 2
VB 2 0 DC 0 PULSE (0 0.8 0 1n 1n 10n 25n)
R 1 3 0.1
 
.temp 100

.TRAN 10p 50n
.op
.PRINT tran I(VC) I(VB)
 
.include mcard_504

.control
run
plot ysep I(VC) I(VB) V(2)
.endc
.END

