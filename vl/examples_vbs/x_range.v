/*
 * Test range select.
 *  dependencies:
 *	register declaration
 *	initial procedural block
 *	assignment statement
 *	system tasks
 */

module main;
	reg [0:7] a;
	reg [0:7] b;
	reg [7:0] c;
	reg [7:0] d;

	initial
		begin
		a = 0;
		b = 0;
		c = 0;
		d = 0;
		a[3:4] = 8'b00000010;
		b[3:4] = 8'b00000001;
		c[4:3] = 8'b00000001;
		d[4:3] = 8'b00000010;
		$write("a=%b(000 10 000) b=%b(000 01 000)\n",a,b);
		$write("c=%b(000 01 000) d=%b(000 10 000)\n",c,d);
		d[6:2] = b[1:3] + c[6:2];
		$write("d=%b (000 01 000)\n",d);
		d[6:6] = 1;
		$write("d=%b (010 01 000)\n",d);
		d[1:1] = d[3:3];
		$write("d=%b (010 01 010)\n",d);

		b = 8'b1010_1000;
		c = 8'b1010_1000;
		$write("c=%b (0101), b=%b (0100)\n", c[6:3], b[3:6]);
		$write("c=%b (1), b=%b (1)\n", c[3:3], b[0:0]);

		b = 0; b[3:4] = 1;		/* b = 0000_1000 */
		c = 0; c[4:3] = 1;		/* c = 0000_1000 */
		d = 0; d[4:3] = 2;		/* a = 0001_0000 */
		$write("b=%b (0000_1000) c=%b (0000_1000) d=%b (0001_0000)\n",
			b,c,d);
		$write("d[6:2] = b[1:3](%b) + c[6:2](%b)\n", b[1:3], c[6:2]);
		/* 000(0) + 00010(2) = 010(2) */
		d[1+5:1+1] = b[1:3] + c[4+2:2];
		$write("d=%b (0000_1000)\n", d);
		b = d;
		$write("b=%b (0000_1000)\n", b);
		end

endmodule
