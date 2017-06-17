/*
 * Test || comparitor.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	if/else statement
 *	blocking assignment
 */

module main ;

	reg [0:7] a;

	initial
		begin
		a = 10;
		if (0 || 0)
			$write("0 || 0 (Not Ok)\n");
		else
			$write("0 || 0 (Ok)\n");
		if (a || 0)
			$write("a || 0 (Ok)\n");
		else
			$write("a || 0 (Not Ok)\n");
		if (234 || a)
			$write("234 || a (Ok)\n");
		else
			$write("234 || a (Not Ok)\n");
		if (a || a)
			$write("a || a (Ok)\n");
		else
			$write("a || a (Not Ok)\n");
		$finish;
		end

endmodule
