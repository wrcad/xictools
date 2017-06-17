/*
 * Test ternary expression.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a, b, c;

	initial
		begin
		a = 5;
		b = 8'b0111_0100;
		c = 8'b1001_0110;
		$write("%b (0111_0100)\n", a ? b : c);
		a = 0;
		$write("%b (1001_0110)\n", a ? b : c);
		a = 8'b0000_00x0;
		$write("%b (xxx1_01x0)\n", a ? b : c);
		$finish;
		end

endmodule
