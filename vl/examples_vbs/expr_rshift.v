/*
 * Test right shift expression.
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
		a = 8'b1000_0000 >> 20;
		b = 8'b0000_0100 >> a;
		$write("%h %h (0x00 0x04)\n", a, b);
		a = b >> 2;
		b = a >> b;
		$write("%h %h (0x01 0x00)\n", a, b);
		$finish;
		end

endmodule
