/*
 * Test bit select.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	assignment statement
 *	system tasks
 */

module main;
	reg [0:3] a;
	reg [3:0] b;

	initial
		begin
		a = 0;
		b = 0;
		a[0] = 1;
		$write("a=%b(1000)\n",a);
		a = 0;
		a[1] = 1;
		$write("a=%b(0100)\n",a);
		a = 0;
		a[2] = 1;
		$write("a=%b(0010)\n",a);
		a = 0;
		a[3] = 1;
		$write("a=%b(0001)\n",a);
		a = 0;
		a[b] = 1;
		$write("a=%b(1000)\n",a);
		a = 0;
		b = 1;
		a[b] = 1;
		$write("a=%b(0100)\n",a);
		a = 0;
		b = 2;
		a[b] = 1;
		$write("a=%b(0010)\n",a);
		a = 0;
		b = 3;
		a[b] = 1;
		$write("a=%b(0001)\n",a);
		end

endmodule
