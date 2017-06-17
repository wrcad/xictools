// Edge triggered D flipflop
module edge_dff(q, qbar, d, clk, clear);

// Inputs and outputs
output q,qbar;
input d, clk, clear;

// Internal variables
wire s, sbar, r, rbar,cbar;

// Data flow statements 
//Create a complement of signal clear
assign cbar = ~clear;

// Input latches
assign  sbar = ~(rbar & s),
	s = ~(sbar & cbar & ~clk),
	r = ~(rbar & ~clk & s),
	rbar = ~(r & cbar & d);

// Output latch
assign  q = ~(s & qbar),
	qbar = ~(q & r & cbar);

endmodule

// Edge triggered T-flipflop. Toggles every clock
// cycle.
module T_ff(q, clk, clear);

// I/O ports
output q;
input clk, clear;

// Instantiate the edge triggered DFF
// Complement of output q is fed back.
// Notice qbar not needed. Empty port.
edge_dff ff1(q, ,~q, clk, clear);

endmodule

// Ripple counter
module counter(Q , clock, clear);

// I/O ports
output [3:0] Q;
input clock, clear;

// Instantiate the T flipflops
T_ff tff0(Q[0], clock, clear);
T_ff tff1(Q[1], Q[0], clear);
T_ff tff2(Q[2], Q[1], clear);
T_ff tff3(Q[3], Q[2], clear);

endmodule
	
// Top level stimulus module
module stimulus;

// Declare variables for stimulating input
reg CLOCK, CLEAR; 
wire [3:0] Q;

initial 
	$monitor($time, " Count Q = %b Clear= %b",  Q[3:0],CLEAR); 

//initial
//	$gr_waves(	"clk", CLOCK,
//			"Clear", CLEAR,
//			"Q", Q[3:0],
//			"Q0", Q[0],
//			"Q1", Q[1],
//			"Q2", Q[2],
//			"Q3", Q[3]);

// Instantiate the design block counter
counter c1(Q, CLOCK, CLEAR);

// Stimulate the Clear Signal
initial
begin
	CLEAR = 1'b1;
	#34 CLEAR = 1'b0;
	#200 CLEAR = 1'b1;
	#50 CLEAR = 1'b0;
end

// Setup the clock to toggle every 10 time units 
initial 
begin 
	CLOCK = 1'b0;
	forever #10 CLOCK = ~CLOCK;
end

// Finish the simulation at time 200
initial
begin
	#400 $finish;
end

endmodule
