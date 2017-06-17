/*
 * Test system tasks and functions.
 *  dependencies:
 *	initial procedural block
 *	sequential block
 *	system tasks
 */

module main;

	initial
		begin
		$write("Hello world.\n");
		$write("%s %d.\n", "Hello again, time =", $time);
		$write("%d %d %d %d (5 6 11 171)\n", 5, 4'b0110, 6'o13, 8'hab);
		$finish;
		end

endmodule
