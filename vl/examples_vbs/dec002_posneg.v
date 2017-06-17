/*
 * Test always procedural block w/ delay or event control.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	delay
 */

module main;

	reg a, b, c;

	initial
		begin
		a = 0;
		b = 0;
		c = 0;
		#1 a = 1;
		b = 1;
		c = 1;
		#1 a = 0;
		b = 0;
		c = 0;
		#1 a = 1;
		b = 1;
		c = 1;
		#1 $finish;
		end

	always @(c)
		$write("%d:  c changed.\n", $time);

	always @(posedge a)
		$write("%d:  posedge a.\n", $time);

	always @(negedge b)
		$write("%d:  negedge b.\n", $time);

	always @(negedge a or posedge b)
		$write("%d:  negedge a or posedge b.\n", $time);

endmodule
