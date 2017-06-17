/*
 * Test logical ^ expression.
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
		a = 8'b0000_1010 ^ 8'b1010_0000;
		b = 8'b1111_0001 ^ a;
		$write("%h %h (0xaa 0x5b)\n", a, b);
		a = b ^ 4;
		b = a ^ b;
		$write("%h %h (0x5f 0x04)\n", a, b);
		$finish;
		end

endmodule
