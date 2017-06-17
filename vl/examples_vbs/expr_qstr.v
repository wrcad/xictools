/*
 * Test not expression.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	system tasks
 *	blocking assignment
 */

module main ;

	reg [0:7] a;
	reg [0:15] b;

	initial
		begin
		a = "a";
		b = "b"; 
		$write("%h %h (0x%h 0x%h)\n", a, b, "a", "b");
		$finish;
		end

endmodule
