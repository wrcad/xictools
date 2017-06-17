/*
 * Test mintypmax expression.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a;

	initial
		begin
		a = (1 : 6 : 14);
		$write("%d (min=1, typ(default)=6, max=14)\n", a);
		$finish;
		end

endmodule
