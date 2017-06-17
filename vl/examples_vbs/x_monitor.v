/*
 * Test system tasks and functions.
 *  dependencies:
 *	initial procedural block
 *	always procedural block
 *	sequential block
 *	system tasks
 */

module main;

	reg a, b, c;

	initial
		begin
		$write(    "            a  b  c\n");
		$monitor("%d:  monitor %d, %d, %d\n", $time, a, b, c);
		$strobe( "%d:  strobe             %d, %d, %d\n", $time, a, b, c);
		a = 0;
		b = 0;
		c = 0;
		#1 a = 1;
		#1 a = 0;
		#1 a = 1;
		#1 a = 0;
		#1 a = 1;
		end

	always @(posedge a)
		b = ~b;

	always @(posedge b)
		c = ~c;

	always @(posedge c)
		a = ~a;

endmodule
