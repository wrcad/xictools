/*
 * Test null delay statement.
 *  dependencies:
 *	initial procedural block
 *	delay control
 *	null statement
 */

module main ;

	initial
		begin
		#1 ;
		$finish;
		end

endmodule
