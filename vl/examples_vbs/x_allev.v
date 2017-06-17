/*
 * Test display of all events.
 *  dependencies:
 *	initial procedural block
 *	always procedural block
 *	system tasks
 */

module main ;

	reg [1:0] a, b, c;

	initial
		begin
		$monitor("%d:  a = %d, b = %d, c = %d\n", $time, a, b, c);
		a = 0;
		b = 0;
		c = 0;
		#1 a = 1;
		#1 $finish;
		end

	always @(posedge a[0])
		begin
		b = 3;
		c = 3;
		end

	always @(b)
		begin
		c <= ~c;
		end

endmodule
