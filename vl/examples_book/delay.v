// Define a simple combination module called D
module D (out, a, b, c);

// I/O port declarations
output out;
input a,b,c;

// Internal nets
wire e;

// Instantiate primitive gates to build the circuit
and #(5) a1(e, a, b);
or  #(4) o1(out, e,c);

endmodule

// Stimulus (top level module)
module stimulus;

// Declare variables
reg A, B, C;
wire OUT;

// Instantiate the module D
D d1( OUT, A, B, C);

// Setup the monitor
initial
	$monitor($time, " A= %b, B=%b, C= %b, OUT= %b\n", A, B, C, OUT);

// Setup the waveform generator. May not be present
// in all Verilog simulators.
//initial
//	$gr_waves("A", A, "B", B, "C", C, "E", d1.e, "OUT", OUT);

// Stimulate the inputs
initial
begin
	A= 1'b0; B= 1'b0; C= 1'b0;

	#10 A= 1'b1; B= 1'b1; C= 1'b1;

	#10 A= 1'b1; B= 1'b0; C= 1'b0;

	#20 $finish;
end

endmodule

