
// Define a 4-bit full adder 
module fulladd4(sum, c_out, a, b, c_in); 
 
// I/O port declarations
output [3:0] sum;
output c_out;
input[3:0] a, b;
input c_in;
 
// Specify the function of a full adder
assign {c_out, sum} = a + b + c_in;

endmodule 

// Define the stimulus (top level module)
module stimulus;

// Set up variables
reg [3:0] A, B;
reg C_IN;
wire [3:0] SUM;
wire C_OUT;

// Instantiate the 4-bit full adder. call it FA1_4
fulladd4 FA1_4(SUM, C_OUT, A, B, C_IN);


// Setup the monitoring for the signal values
initial
begin
	$monitor($time," A= %b, B=%b, C_IN= %b,, C_OUT= %b, SUM= %b\n",
													A, B, C_IN, C_OUT, SUM);
end

// Stimulate inputs
initial
begin
	A = 4'd0; B = 4'd0; C_IN = 1'b0;

	#5 A = 4'd3; B = 4'd4;

	#5 A = 4'd2; B = 4'd5;
	
	#5 A = 4'd9; B = 4'd9;
	
	#5 A = 4'd10; B = 4'd15;

	#5 A = 4'd10; B = 4'd5; C_IN = 1'b1;
end	

endmodule
