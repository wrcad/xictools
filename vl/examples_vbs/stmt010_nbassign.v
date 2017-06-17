/*
 * Test blocking assignment.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 */

module main;

	reg a, b;

	initial
		begin
		a = 0;
		b = 1;
		$monitor("%d:  a = %d, b = %d\n", $time, a, b);
		#5 $finish;
		end

	always #1
		begin
		a <= ~a;
		b <= ~b;
		end

endmodule
