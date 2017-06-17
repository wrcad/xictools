/*
 * Test always procedural block.
 *  dependencies:
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	delay
 */

module main;

	initial
		begin
		#10 $finish;
		end

	always #1
		$write("In always statement, time = %d\n", $time);

endmodule
