module edge_dff(q, d, clk);

output q;
reg q;
input d, clk;

always @(posedge clk)
	q <= d;
endmodule

	
