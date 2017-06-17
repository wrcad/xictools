/*
 * Test delays after the equal sign.
 *  dependencies:
 *	initial procedural block
 *	delay control
 *	blocking assignment
 *	invert expression
 *	system task
 */

module main ;

	reg a, b;

	initial
		begin
		$monitor("%d:  a = %d\n", $time, a);
		$monitor("%d:  b = %d\n", $time, b);
		a = #1 0;
		#1 a = #2 ~a;
		b = #0 1;
		#3 $finish;
		end

endmodule
