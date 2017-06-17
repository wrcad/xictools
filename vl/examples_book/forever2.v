
module synchronize;

//Example 2: Synchronize two register values at every positive edge of 
//clock
reg clock;
reg x, y;

initial
begin
		clock = 1'b0;
		x = 1'b0;
		y = 1'b0;
		#100000 $finish;
end

always #5 clock = ~clock;
always #11 y = ~y;

initial
        forever @(posedge clock) x = y;

endmodule

