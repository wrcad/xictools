/*
 * Test bases.
 *  dependencies:
 *	number
 */

module main;
	reg [0:15] b;
	reg [15:0] a;

	initial
		begin
		a = 16'habcd;
		b = 43981;
		$write("%s (a=0xabcd, b=43981...\n",
			"numbers are 16 bits wide");
		$write("a %h %d %o %b (not specified)\n",a,a,a,a);
		$write("a %3h %10d %7o %16b (3/10/7/16)\n",a,a,a,a);
		$write("b %h %d %o %b (not specified)\n",b,b,b,b);
		$write("b %3h %10d %7o %16b (3/10/7/16)\n",b,b,b,b);
		end
endmodule
