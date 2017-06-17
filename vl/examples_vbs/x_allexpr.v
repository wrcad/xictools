/*
 * Test bit select.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	assignment statement
 *	system tasks
 */

module main;
	reg [0:3] a;
	reg [3:0] b;
	reg [4:1] c;
	reg [1:4] d;

	initial
		begin
		a = 1;
		b = 2;
		c = 3;
		d = ~(a & b | c);
		$write("d = %d (12)\n", d);
		end

endmodule
