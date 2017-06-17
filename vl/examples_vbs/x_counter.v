/*
 * Test add expression.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	always procedural block
 *	instantiation
 *	system tasks (monitor)
 *	blocking assignment
 */

module counter (clk, cnt, reset, enable, updn);
	input clk, reset, enable, updn;
	output [3:0] cnt;
	reg [3:0] cnt;

	always @(posedge clk)
		begin
		if (enable)
			begin
			if (updn)
				cnt = cnt+1;
			if (!updn)
				cnt = cnt-1;
			end
		if (reset)
			cnt = 0;
		end

endmodule

module main;
	wire [0:3] cnt;
	reg clk, reset, enable, updn;

	counter ct(clk, cnt, reset, enable, updn);

	initial
		begin
		$monitor("%d:  %d\n", $time, cnt);
		reset = 0;
		cnt = 0;
		clk = 0;
		enable = 1;
		updn = 1;
		#2 reset = 1;
		#2 reset = 0;
		#14 enable = 0;
		#5 enable = 1;
		updn = 0;
		#34 $finish;
		end

	always #1 clk = ~clk;

endmodule
