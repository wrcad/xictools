/*
 * Test add expression.
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
		a = 1 + 2;
		b = 3 + a;
		$write("%h %h (0x3 0x6)\n", a, b);
		a = b + 4;
		b = a + b;
		$write("%h %h (0xa 0x10)\n", a, b);
		$finish;
		end

endmodule
