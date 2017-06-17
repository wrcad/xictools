/*
 * Test parameter declaration.
 *  dependencies:
 *	parameter declaration
 *	register declaration
 *	initial procedural block
 *	system task enable
 */

module main;

	parameter a = 8, b = 20, c = 0;
	reg d [a:b];

	initial
		begin
		$write("a = %d, b = %d, c = %d\n", a, b, c);
		end

endmodule
