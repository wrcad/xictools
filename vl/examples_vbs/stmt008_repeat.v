/*
 * Test repeat loop.
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
		a = 5;
		repeat (5)
			begin
			$write("a = %d\n", a);
			a = a - 1;
			end
		$finish;
		end

endmodule
