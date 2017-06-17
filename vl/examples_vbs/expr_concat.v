/*
 * Test concatenation.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a;
	reg [0:11] b;

	initial
		begin
		a = 5;
		b = { a , 4'hd };
		$write("%h %h (0x5d 0xb3)\n", b, { 4'hb, 4'b0011 });
		end

endmodule
