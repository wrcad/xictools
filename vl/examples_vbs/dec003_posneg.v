/*
 * Test initial procedural block w/ delay or event control.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	sequential block
 *	system task/function
 *	delay
 */

module main;

	reg a;

	initial
		begin
		a = 0;
		@(negedge a) $write("%d:  negedge a\n", $time);
		@(posedge a) $write("%d:  posedge a\n", $time);
		end

	initial
		begin
		#1 a = 1;
		#2 a = 0;
		#3 a = 1;
		#4 a = 0;
		#5 a = 1;
		#6 a = 0;
		end

endmodule
