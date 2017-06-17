/*
 * Test delay statements.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 */

module main ;

	initial
		begin
		#1 $write("current time: %d\n", $time);
		#5 $write("current time: %d\n", $time);
		#10 $write("current time: %d\n", $time);
		#15 $write("current time: %d\n", $time);
		$finish;
		end

endmodule
