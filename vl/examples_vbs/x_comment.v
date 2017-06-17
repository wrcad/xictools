/*
 * Test comment.
 *  dependencies:
 *	parser
 */

module main;
	reg [0:7] a,b,c;

	initial
		begin
		#1 a = 0; /* initialize variables */
		#1 b = 245; // another test for comment!
		#1 c = 10;
		/*
		 * We now support multiline comments.
		 * This is a test of multiline comments.
		 */
		#1 $write("Passed the comment test!\n");
		#1 $finish;
		end

endmodule
