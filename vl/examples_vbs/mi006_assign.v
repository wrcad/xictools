/*
 * Test always procedural block.
 *  dependencies:
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	delay
 */

module main;

	wire [7:0] a, b, c;
	wire d;

	assign a = b + c;

	assign d = 1'b1;

	initial
		begin
		$monitor("%d:  a = %d d = %b\n", $time, a, d);
		b = 0;
		c = 0;
		#1 b = 1;
		#1 c = 1;
		#1 b = 2;
		#1 c = 2;
		#1 $finish;
		end

endmodule
