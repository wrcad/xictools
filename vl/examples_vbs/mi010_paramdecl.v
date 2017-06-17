/*
 * Test parameter declaration.
 *  dependencies:
 *	parameter declaration
 *	register declaration
 *	initial procedural block
 *	system task enable
 */

module sample;

	parameter a = 8, b = 20, c = 0;
	reg d [a:b];

	initial
		begin
		$write("a = %d, b = %d, c = %d\n", a, b, c);
		end

endmodule

module main;

	sample #(9,99) sample1();

endmodule
