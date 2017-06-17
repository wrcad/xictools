/*
 * Test >= comparitor.
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
		if (123 >= 123)
			$write("123 >= 123 (Ok)\n");
		else
			$write("123 >= 123 (Not Ok)\n");
		if (a >= 0)
			$write("a >= 0 (Ok)\n");
		else
			$write("a >= 0 (Not Ok)\n");
		if (2 >= a)
			$write("2 >= a (Not Ok)\n");
		else
			$write("2 >= a (Ok)\n");
		if (a >= a)
			$write("a >= a (Ok)\n");
		else
			$write("a >= a (Not Ok)\n");
		$finish;
		end

endmodule
