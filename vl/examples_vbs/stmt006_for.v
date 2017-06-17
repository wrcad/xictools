/*
 * Test for loop.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 *	subtract expression
 */

module main ;

	reg [0:7] a;

	initial
		begin
		for (a = 5; a ; a = a - 1)
			$write("a = %d\n", a);
		$finish;
		end

endmodule
