/*
 * Test delayed loop.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	for statement
 */

module main ;

	reg [3:0] i;

	initial
		begin
		i = 0;
		#1 $write("%d:  Starting loop.\n", $time);
		for (i = 0; i < 5; i = i + 1)
			begin
			$write("%d:  First line in loop.\n", $time);
			#1 $write("%d:  Second line in loop.\n", $time);
			$write("%d:  Third line in loop.\n", $time);
			end
		#1 $write("%d:  Ending loop.\n", $time);
		$finish;
		end

endmodule
