module latch(q, d, clk);

output q;
reg q;
input d, clk;

always @(clk or d)
 	if (clk)
		q <= d;
endmodule

	
