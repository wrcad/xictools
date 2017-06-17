/*
 * Test left shift expression.
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
		a = 8'b0000_0010 << 1;
		b = 8'b0000_0011 << a;
		$write("%h %h (0x04 0x30)\n", a, b);
		a = b << 2;
		b = a << b;
		$write("%h %h (0xc0 0x00)\n", a, b);
		$finish;
		end

endmodule
