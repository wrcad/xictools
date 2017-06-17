
// This example illustrates the different components of a module

// Module name and port list
// SR_latch module
module SR_latch(Q, Qbar, Sbar, Rbar);

//Port declarations
output Q, Qbar;
input Sbar, Rbar;
 
// Instantiate lower level modules
// In this case, instantiate Verilog primitive "nand" gates
// Note, how the wires are connected in a cross coupled fashion.
nand n1(Q, Sbar, Qbar);
nand n2(Qbar, Rbar, Q);

// endmodule statement
endmodule

// Module name and port list
// Stimulus module 
module Top;

// Declarations of wire, reg and other variables
wire q, qbar;
reg set, reset;

// Instantiate lower level modules
// In this case, instantiate SR_latch
SR_latch l1(q, qbar, ~set, ~reset);

// Behavioral block, initial
initial
begin
  $monitor($time, " set = %b, reset= %b, q= %b\n",set,reset,q);
  set = 0; reset = 0;
  #5 reset = 1;
  #5 reset = 0;
  #5 set = 1;
  #5 set = 0; reset = 1;
  #5 set = 0; reset = 0;
  #5 set = 1; reset = 0;
end
 
// endmodule statement
endmodule
