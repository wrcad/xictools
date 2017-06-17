/*
 * Test integer declaration.
 *  dependencies:
 *	integer declaration
 *	initial procedural block
 *	system task enable
 */

module main;

	integer a, b, c;

	initial
		begin
		a = 1234;
		b = 23456789;
		c = 0;
		$write("a = %d, b = %d, c = %d\n", a, b, c);
		end

endmodule
