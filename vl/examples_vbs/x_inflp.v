/*
 * Test infinite loop.  Do not simulate this module, it will cause an
 * infinite loop.
 *  dependencies:
 *	initial procedural block
 *	always procedural block
 *	system tasks
 */

module main ;

	reg a, b, c;

	initial
		begin
		$monitor("%d:  a = %d, b = %d, c = %d\n", $time, a, b, c);
		a = 0;
		b = 0;
		c = 1;
		#1 a = 1;
		#1 $finish;
		end

	always @(posedge a)
		begin
		c <= b;
		b <= c;
		end

endmodule
