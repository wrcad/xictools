/*
 * Test logical & expression.
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
		a = 8'b1111_1010 & 8'b1010_1111;
		b = 8'b0101_1010 & a;
		$write("%h %h (0xaa 0x0a)\n", a, b);
		a = b & 7;
		b = a & b;
		$write("%h %h (0x02 0x02)\n", a, b);
		$finish;
		end

endmodule
