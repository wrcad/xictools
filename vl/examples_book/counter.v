//Binary  counter
module counter(Q , clock, clear);

// I/O ports
output [3:0] Q;
input clock, clear;
//output defined as register
reg [3:0] Q;

always @( posedge clear  or negedge clock)
begin
	if (clear)
	   Q = 4'd0;
	else
	   //Q = (Q + 1) % 16;
	   Q = (Q + 1) ;
end

endmodule
	
// Top level stimulus module
module stimulus;

// Declare variables for stimulating input
reg CLOCK, CLEAR; 
wire [3:0] Q;

initial 
	$monitor($time, " Count Q = %b Clear= %b",  Q[3:0],CLEAR); 

/*
initial
	$gr_waves(	"clk", CLOCK,
			"Clear", CLEAR,
			"Q", Q[3:0],
			"Q0", Q[0],
			"Q1", Q[1],
			"Q2", Q[2],
			"Q3", Q[3]);
*/

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
	#400 $stop;
end

endmodule
