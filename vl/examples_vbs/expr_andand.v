/*
 * Test && comparitor.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system task
 *	blocking assignment
 *	if/else statement
 */

module main;

	reg [0:7] a;

	initial
		begin
		a = 10;
		if (34 && 12)
			$write("34 && 12 (Ok)\n");
		else
			$write("34 && 12 (Not Ok)\n");
		if (a && 0)
			$write("a && 0 (Not Ok)\n");
		else
			$write("a && 0 (Ok)\n");
		if (234 && a)
			$write("234 && a (Ok)\n");
		else
			$write("234 && a (Not Ok)\n");
		if (a && a)
			$write("a && a (Ok)\n");
		else
			$write("a && a (Not Ok)\n");
		$finish;
		end

endmodule
