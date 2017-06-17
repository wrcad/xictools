module multiple (q, clk, load1, load2, a1, a2);

output q;
reg q;
input clk, load1, load2, a1, a2;

always @(posedge clk)
	if(load1) q <= a1;
always @(posedge clk)
	if(load2) q <= a2;

endmodule
