/*
 * Test nested delays.
 *  dependencies:
 *	initial procedural block
 *	system tasks
 *	if statement
 */

module main ;

	initial
		begin
		#1 $write("%d:  Delayed 1 time unit.\n", $time);
		#1 $write("%d:  Delayed 2 time unit.\n", $time);
		if (1)
			begin
			#1 $write("%d:  Delayed 3 time unit.\n", $time);
			#1 $write("%d:  Delayed 4 time unit.\n", $time);
			if (1)
				begin
				#1 $write("%d:  Delayed 5 time unit.\n", $time);
				#1 $write("%d:  Delayed 6 time unit.\n", $time);
				end
			#1 $write("%d:  Delayed 7 time unit.\n", $time);
			#1 $write("%d:  Delayed 8 time unit.\n", $time);
			if (1)
				begin
				#1 $write("%d:  Delayed 9 time unit.\n", $time);
				#1 $write("%d:  Delayed 10 time unit.\n", $time);
				end
			#1 $write("%d:  Delayed 11 time unit.\n", $time);
			#1 $write("%d:  Delayed 12 time unit.\n", $time);
			end
		#1 $write("%d:  Delayed 13 time unit.\n", $time);
		#1 $write("%d:  Delayed 14 time unit.\n", $time);
		if (1)
			begin
			#1 $write("%d:  Delayed 15 time unit.\n", $time);
			#1 $write("%d:  Delayed 16 time unit.\n", $time);
			end
		#1 $write("%d:  Delayed 17 time unit.\n", $time);
		#1 $write("%d:  Delayed 18 time unit.\n", $time);
		#1 $finish;
		end

endmodule
