/*
 * Test integer declaration.
 *  dependencies:
 *	memory declaration
 *	initial procedural block
 *	system task enable
 */

module main;

	reg a [0:2];
	reg [1:0] b [2:0];
	reg [1:6] c [2:5];

	initial
		begin
		a[1] = 1;
		b[2] = 2;
		c[3] = 33;
		$write("a[1] = %d (1)\n", a[1]);
		$write("b[2] = %d (2)\n", b[2]);
		$write("c[3] = %d (33)\n", c[3]);
		$write("a[0] = %d (x)\n", a[0]);
		$write("b[1] = %d (x)\n", b[1]);
		$write("c[2] = %d (x)\n", c[2]);
		end

endmodule
