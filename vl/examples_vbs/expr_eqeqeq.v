/*
 * Test === comparitor.
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
		a = 8'b1x0x_10xx;
		if (4'b1x01 === 5'b01x01)
			$write("1x01 === 1x01 (Ok)\n");
		else
			$write("1x01 === 1x01 (Not Ok)\n");
		if (a === 8'b1x0x_1x0x)
			$write("a === 1x0x_1x0x (Not Ok)\n");
		else
			$write("a === 1x0x_1x0x (Ok)\n");
		if (8'b1x0x_1x0x === a)
			$write("1x0x_1x0x === a (Not Ok)\n");
		else
			$write("1x0x_1x0x === a (Ok)\n");
		if (a === a)
			$write("a === a (Ok)\n");
		else
			$write("a === a (Not Ok)\n");
		end

endmodule
