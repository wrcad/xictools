**** DC OP analysis of mextram 504 model ****
.OPTIONs GMIN=1.0e-13

Q1 1 2 3 4 m504 $ area=1 m=1

* Start sources
VB 2 0 DC 1.2
VC 1 0 DC 2.2
VE 3 0 DC 0.0
VS 4 0 DC 0.0

.DC VB 0.4 1.2 0.1
.DC VC 1.4 2.2 0.1
 
.op
.PRINT DC I(VC) I(VB) I(VE) I(VS)
.TEMP 25
 
.include mcard_504

.control
run
print I(VC) I(VB) I(VE) I(VS)
.endc
.END

