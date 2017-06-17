/*
 * Test forever loop.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 *	if statement
 *	subtract expression
 */

module main ;

	reg [0:7] a;

	initial
		begin
		a = 5;
		forever
			begin
			$write("a = %d\n", a);
			if (a == 0)
				$finish;
			a = a - 1;
			end
		end

endmodule
