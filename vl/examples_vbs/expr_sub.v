/*
 * Test sub expression.
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
		a = 2 - 1;
		b = 10 - a;
		$write("%h %h (0x1 0x9)\n", a, b);
		a = b - 3;
		b = a - b;
		$write("%h %h (0x6 0xfd)\n", a, b);
		$finish;
		end

endmodule
