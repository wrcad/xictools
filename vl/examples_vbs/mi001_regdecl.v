/*
 * Test register declaration.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system task enable
 */

module main;

	reg this_is_a_variable, this_is_another_variable;
	reg [1:4] this_is_a_range_variable;
	reg 	a,
		b,
		c;
	reg [7:0] d,
		  e,
		  f;

	initial
		begin
		$finish;
		end

endmodule
