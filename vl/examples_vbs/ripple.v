// module DFF with synchronous reset
module DFF(q, d, clk, reset);

output q; 
input d, clk, reset;
reg q;

always @(posedge reset or negedge clk)
begin
if (reset)
		q = 1'b0;
else
		q = d;
end

endmodule

module TFF(q, clk, reset);

output q; 
input clk, reset;
wire d;

DFF dff0(q, d, clk, reset); 
not n1(d, q); // not is a Verilog provided primitive.

endmodule

module ripple_carry_counter(q, clk, reset);

output [3:0] q; 
input clk, reset; 

//4 instances of the module TFF are created. 
TFF tff0(q[0],clk, reset);
TFF tff1(q[1],q[0], reset);
TFF tff2(q[2],q[1], reset);
TFF tff3(q[3],q[2], reset);

endmodule


module stimulus;

reg clk;
reg reset;
wire[3:0] q;

// instantiate the design block
ripple_carry_counter r1(q, clk, reset);

// Control the clk signal that drives the design block.
initial clk = 1'b0;
always #5 clk = ~clk;

// Control the reset signal that drives the design block
initial
begin
reset = 1'b1;
#15 reset = 1'b0;
#180 reset = 1'b1;
#10 reset = 1'b0;
#20 $stop;
end

// Monitor the outputs 
initial
    $monitor($time, " Output q = %d\n", q);

endmodule

