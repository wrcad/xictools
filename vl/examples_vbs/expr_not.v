/*
 * Test not expression.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a, b;

	initial
		begin
		a = !10;
		b = !a;
		$write("%h %h (0x00 0x01)\n", a, b);
		$finish;
		end

endmodule
